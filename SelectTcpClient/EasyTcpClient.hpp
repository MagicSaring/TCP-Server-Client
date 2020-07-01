#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#define mystrcpy(a,b) strcpy_s(a, sizeof(a), b);
#pragma comment(lib, "WS2_32.LIB")

#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define	SOCKET					int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define mystrcpy(a,b) strcpy(a, b);
#endif 

#include <stdio.h>
#include <thread>
#include "MessageHeader.hpp"


class EasyTcpClient
{
public:
	EasyTcpClient()
	{
		m_hSocket = INVALID_SOCKET;
	}
	//虚析构函数
	virtual ~EasyTcpClient()
	{
		Close();
	}

	int InitSocket();	//初始化socket
	int Connect(const char* ip, short port);	//连接服务器
	void Close();	//关闭socket
	int RecvData();	//收数据
	void OnNetMsg(DataHeader *header);	//处理网络消息
	int SendData(DataHeader* header);	//发送数据
	bool OnRun();	//处理网络消息
	bool isRun();	//是否运行中

private:
	SOCKET m_hSocket;

};

int EasyTcpClient::InitSocket()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);		//生成版本号
	WSADATA data;

	//启动Windows socket 2.x环境
	if (WSAStartup(ver, &data) != 0)
	{
		printf("错误,加载winsock.dll失败...错误代码:%d\n", WSAGetLastError());
		return -1;
	}
#endif // _WIN32

	//1.创建一个socket套接字
	if (INVALID_SOCKET != m_hSocket)
	{
		printf("<socket=%d>关闭旧连接..\n",m_hSocket);
		Close();
	}

	m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET)
	{
#ifdef _WIN32
		printf("错误,建立Socket失败...错误代码:%d\n", WSAGetLastError());
		WSACleanup();
#else
		printf("错误,建立Socket失败.....\n");
#endif
		return -1;
	}
	printf("建立Socket=<%d>成功\n",m_hSocket);

	return 0;
}

int EasyTcpClient::Connect(const char* ip, short port)
{
	if (INVALID_SOCKET == m_hSocket)
	{
		if(SOCKET_ERROR == InitSocket())
		{
			return -1;
		}
	}

	//2. 连接服务器 connect
	struct sockaddr_in server_addr;
	int addr_len = sizeof(struct sockaddr_in);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip);

	int ret = connect(m_hSocket, (struct sockaddr*)&server_addr, addr_len);
	if (ret == SOCKET_ERROR)
	{
#ifdef _WIN32
		printf("错误,Socket=<%d>连接服务器<ip:%s,port:%d>失败...错误代码:%d\n", m_hSocket, ip, port, WSAGetLastError());
		closesocket(m_hSocket);
		WSACleanup();
#else
		printf("错误,连接服务器失败...\n");
		close(m_hSocket);
#endif // _WIN32
	}
	else
	{
		printf("Socket=<%d>连接服务器<ip:%s,port:%d>成功...\n", m_hSocket, ip, port);
	}

	return ret;
}

void EasyTcpClient::Close()
{
	if (m_hSocket != INVALID_SOCKET)
	{
#ifdef _WIN32
		//7.关闭套接字 closesocket
		closesocket(m_hSocket);
		//8.清除windows socket环境
		WSACleanup();
#else
		//7.关闭套接字 closesocket
		close(m_hSocket);
#endif // _WIN32
		m_hSocket = INVALID_SOCKET;
	}
}

//接收数据,处理粘包,拆分包
int EasyTcpClient::RecvData()
{
	//缓冲区
	char szRecv[4096] = {0};

	// 5.接受客户端的请求数据
	int nLen = (int)recv(m_hSocket, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("与服务端断开连接,客户端<Socket=%d>已退出,任务结束\n", m_hSocket);
		Close();
		return -1;
	}

	DataHeader* header = (DataHeader*)szRecv;
	recv(m_hSocket, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
	OnNetMsg(header);

	return 0;
}

//响应
void EasyTcpClient::OnNetMsg(DataHeader* header)
{
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		LoginResult* loginRet = (LoginResult*)header;
		printf("客户端<Socket=%d>收到服务端消息命令:CMD_LOGIN_RESULT, 数据长度:%d\n", m_hSocket, loginRet->dataLength);
		printf("LoginResult:%d\n", loginRet->result);

	}break;
	case CMD_LOGOUT_RESULT:
	{
		LogoutResult* logoutRet = (LogoutResult*)header;

		printf("客户端<Socket=%d>收到服务端消息命令:CMD_LOGOUT_RESULT, 数据长度:%d\n", m_hSocket, logoutRet->dataLength);
		printf("LogoutResult:%d\n", logoutRet->result);

	}break;
	case CMD_NEW_USER_JOIN:
	{
		NewUserJoin* userJoin = (NewUserJoin*)header;

		printf("客户端<Socket=%d>收到服务端消息命令:CMD_NEW_USER_JOIN, 数据长度:%d\n", m_hSocket, userJoin->dataLength);
		printf("sock:%d\n", userJoin->sock);
	}break;
	}
}

int EasyTcpClient::SendData(DataHeader* header)
{
	if (isRun() && header)
	{
		return send(m_hSocket, (const char*)header, header->dataLength, 0);
	}
	return SOCKET_ERROR;
}

bool EasyTcpClient::OnRun()
{
	if (isRun())
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);					//FD_ZERO:清空一个文件描述符集合
		FD_SET(m_hSocket, &fdReads);		//FD_SET:将监听的文件描述符，添加到监听集合中

		/*
		select函数原型:
		int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);

		参数列表:
		nfds:		监听的所有文件描述符的最大描述符 + 1(内核采取轮询的方式)
		readfds:	读文件描述监听集合
		writefds:	写文件描述符集合
		exceptfds:	异常文件描述符集合
		timeout:	大于0:设置监听超时时长;NULL:阻塞监听;0:非阻塞监听

		返回值:
		大于0:所欲监听集合中,满足对应时间的总数
		0:没有满足的
		-1:出错error
		*/

		timeval t = { 0, 0 };
		int ret = select(m_hSocket + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			printf("客户端<Socket=%d>select任务结束\n", m_hSocket);
			Close();
			return false;
		}

		//FD_ISSET:判断一个文件描述符是否在一个集合中，返回值:在1,不在0
		if (FD_ISSET(m_hSocket, &fdReads))
		{
			//FD_CLR:将一个文件描述符从集合中移除
			FD_CLR(m_hSocket, &fdReads);

			if (-1 == RecvData())
			{
				printf("客户端<Socket=%d>select任务结束\n", m_hSocket);
				Close();
				return false;
			}
		}
		return true;
	}
	return false;
}

bool EasyTcpClient::isRun()
{
	return m_hSocket != INVALID_SOCKET;
}

#endif
