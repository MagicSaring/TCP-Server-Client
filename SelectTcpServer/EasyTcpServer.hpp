#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

using namespace std;
#include <stdio.h>
#include <vector>
#include <algorithm>
#include "MessageHeader.hpp"
#define  PORT 4567

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.LIB")

#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define	SOCKET					int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#endif 

#include <thread>

#ifndef RECV_BUFF_SIZE
#define  RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}
	char* msgBuf()
	{
		return _szMsgBuf;
	}
	int getLastPos()
	{
		return _lastPos;
	}

	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

private:
	SOCKET _sockfd;	//文件描述符

	//第二缓冲区,消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];

	//消息缓冲区的数据尾部位置
	int _lastPos;
};

class EasyTcpServer
{
public:
	EasyTcpServer()
	{
		m_hSocket = INVALID_SOCKET;
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}

	//初始化Socket
	SOCKET InitSocket();

	//绑定IP和端口号
	int Bind(const char* ip, unsigned short port);

	//监听端口号
	int Listen(int n);

	//接受客户端连接
	SOCKET Accept();

	//关闭Socket
	int Close();

	//处理网络消息
	bool OnRun();

	//是否工作中
	bool IsRun();

	//接收数据,处理粘包,拆分包
	int RecvData(ClientSocket* pClient);

	//响应网络消息
	int OnNetMsg(SOCKET sock, DataHeader* header);

	//发送指定数据
	int SendData(SOCKET sock, DataHeader* header);

	//群发数据
	int BatchSendData(DataHeader* header);

private:
	SOCKET m_hSocket;
	vector<ClientSocket*> _clients;
};

SOCKET EasyTcpServer::InitSocket()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);		//生成版本号
	WSADATA data;

	//启动Windows socket 2.x环境
	if (WSAStartup(ver, &data) != 0)
	{
		printf("错误,加载winsock.dll失败...错误代码:%d", WSAGetLastError());
		return INVALID_SOCKET;
	}
#endif // _WIN32

	if (INVALID_SOCKET != m_hSocket)
	{
		printf("<socket=%d>关闭旧连接..\n", m_hSocket);
		Close();
	}

	//创建一个socket套接字
	m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET)
	{
#ifdef _WIN32
		printf("错误,建立Socket失败...错误代码:%d", WSAGetLastError());
#else
		printf("错误,建立Socket失败...\n");
#endif // _WIN32
	}
	else
	{
		printf("建立<Socket=%d>成功...\n", (int)m_hSocket);
	}

	return m_hSocket;
}

int EasyTcpServer::Bind(const char* ip, unsigned short port)
{
	struct sockaddr_in server_addr = {};
	int addr_len = sizeof(struct sockaddr_in);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (ip)
	{
		server_addr.sin_addr.s_addr = inet_addr(ip);
	}
	else
	{
		server_addr.sin_addr.s_addr = INADDR_ANY;
	}

	int ret = bind(m_hSocket, (struct sockaddr*)&server_addr, addr_len);
	if (SOCKET_ERROR == ret)
	{
#ifdef _WIN32
		printf("错误,<Socket=%d>绑定用于接受客户端连接的网络端口<%d>失败...错误代码:%d\n", (int)m_hSocket, port, WSAGetLastError());
#else
		printf("错误,<Socket=%d>绑定用于接受客户端连接的网络接口<%d>失败...\n", (int)m_hSocket, port);
#endif // _WIN32
	}
	else
	{
		printf("<Socket=%d>绑定网络端口<%d>成功\n", (int)m_hSocket, port);
	}

	return ret;
}

int EasyTcpServer::Listen(int n)
{
	int ret = listen(m_hSocket, n);
	if (SOCKET_ERROR == ret)
	{
#ifdef _WIN32
		printf("错误,<Socket=%d>监听网络端口失败...错误代码:%d\n", (int)m_hSocket, WSAGetLastError());
#else
		printf("错误,<Socket=%d>监听网络端口失败...\n", (int)m_hSocket);
#endif // _WIN32

	}
	else
	{
		printf("<Socket=%d>监听网络端口成功...\n", (int)m_hSocket);
	}

	return ret;
}

SOCKET EasyTcpServer::Accept()
{
	struct sockaddr_in client_addr = {};
	SOCKET new_socket = INVALID_SOCKET;
	int addr_len = sizeof(struct sockaddr_in);

#ifdef _WIN32
	new_socket = accept(m_hSocket, (struct sockaddr*)&client_addr, &addr_len);
#else
	new_socket = accept(m_hSocket, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
#endif // _WIN32

	if (new_socket == INVALID_SOCKET)
	{
#ifdef _WIN32
		printf("错误,<Socket=%d>接受到无效客户端Socket...错误代码:%d\n", (int)m_hSocket, WSAGetLastError());
#else		
		printf("错误,<Socket=%d>接受到无效客户端Socket\n", (int)m_hSocket);
#endif // _WIN32
	}
	else
	{
		NewUserJoin userJoin;
		BatchSendData(&userJoin);
		_clients.push_back(new ClientSocket(new_socket));
		printf("<Socket=%d>新客户端加入: Socket = %d, IP = %s\n", (int)m_hSocket, (int)new_socket, inet_ntoa(client_addr.sin_addr));
	}

	return new_socket;
}

int EasyTcpServer::Close()
{
	if (m_hSocket != INVALID_SOCKET)
	{
#ifdef _WIN32
		for (size_t i = 0; i < _clients.size(); ++i)
		{
			closesocket(_clients[i]->sockfd());
			delete _clients[i];
		}

		closesocket(m_hSocket);

		//清除Windows socket环境
		WSACleanup();
#else
		for (size_t i = 0; i < _clients.size(); ++i)
		{
			close(_clients[i]->sockfd());
			delete _clients[i];
		}

		close(m_hSocket);
#endif // _WIN32

		_clients.clear();
	}

	return 0;
}

bool EasyTcpServer::OnRun()
{
	if (IsRun())
	{
		//伯克利 socket
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		//FD_ZERO:清空一个文件描述符集合
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		//FD_SET:将监听的文件描述符，添加到监听集合中
		FD_SET(m_hSocket, &fdRead);
		FD_SET(m_hSocket, &fdWrite);
		FD_SET(m_hSocket, &fdExp);

		SOCKET maxSocket = m_hSocket;

		for (size_t i = 0; i < _clients.size(); ++i)
		{
			FD_SET(_clients[i]->sockfd(), &fdRead);
			if (_clients[i]->sockfd() > maxSocket)
			{
				maxSocket = _clients[i]->sockfd();
			}
		}

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
		timeval t = { 1,0 };
		int ret = select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
		/*printf("select ret = %d count = %d\n", ret, count ++);*/
		if (ret < 0)
		{
			printf("select任务结束\n");
			Close();
			return false;
		}

		//FD_ISSET:判断一个文件描述符是否在一个集合中，返回值:在1,不在0
		if (FD_ISSET(m_hSocket, &fdRead))
		{
			//FD_CLR:将一个文件描述符从集合中移除
			FD_CLR(m_hSocket, &fdRead);

			Accept();
		}

		for (size_t i = 0; i < _clients.size(); ++i)
		{
			if (FD_ISSET(_clients[i]->sockfd(), &fdRead))
			{
				if (-1 == RecvData(_clients[i]))
				{
					auto iter = find(_clients.begin(), _clients.end(), _clients[i]);
					if (iter != _clients.end())
					{
						delete _clients[i];
						_clients.erase(iter);
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool EasyTcpServer::IsRun()
{
	return m_hSocket != INVALID_SOCKET;
}

int EasyTcpServer::RecvData(ClientSocket* pClient)
{
	char _szRecv[RECV_BUFF_SIZE] = {0};

	// 5.接受客户端的请求数据
	int nLen = (int)recv(pClient->sockfd(), _szRecv, sizeof(_szRecv), 0);
	if (nLen <= 0)
	{
		printf("客户端<Socket=%d>已退出,任务结束\n", (int)pClient->sockfd());
		return -1;
	}

	//将收取到的数据拷贝到消息缓冲区
	memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);

	//消息缓冲区的数据尾部位置后移
	pClient->setLastPos(pClient->getLastPos() + nLen);

	//判断消息缓冲区的数据长度大于消息头DataHeader长度
	while (pClient->getLastPos() >= sizeof(DataHeader))
	{
		DataHeader* header = (DataHeader*)pClient->msgBuf();

		//判断消息缓冲区的数据长度大于消息长度
		if (pClient->getLastPos() >= header->dataLength)
		{
			//消息缓冲区剩余未处理数据的长度
			int nSize = pClient->getLastPos() - header->dataLength;

			//处理网络消息
			OnNetMsg(pClient->sockfd(), header);

			//将消息缓冲区剩余未处理数据前移
			memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);

			//消息缓冲区的数据尾部位置前移
			pClient->setLastPos(nSize);
		}
		else
		{
			//消息缓冲区剩余数据不够一条完整消息
			break;
		}
	}

	return 0;
}

int EasyTcpServer::OnNetMsg(SOCKET sock, DataHeader* header)
{
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		Login* login = (Login*)header;
		//printf("收到客户端<Socket=%d>命令:CMD_LOGIN, 数据长度;%d\n", (int)sock, login->dataLength);
		//printf("用户登录...用户姓名:%s, 密码:%s\n", login->userName, login->passWord);

		LoginResult ret;
		SendData(sock, &ret);
	}break;
	case CMD_LOGOUT:
	{
		Logout* logout = (Logout*)header;
		//printf("收到客户端<Socket=%d>命令:CMD_LOGOUT, 数据长度;%d\n", (int)sock, logout->dataLength);
		//printf("用户登出...用户姓名:%s\n", logout->userName);

		LogoutResult ret;
		SendData(sock, &ret);
	}break;
	default:
	{
		printf("收到客户端<Socket=%d>未定义消息, 数据长度:%d\n", (int)sock, header->dataLength);

		//DataHeader ret;
		//SendData(sock, &ret);

	}break;
	}

	return 0;
}

int EasyTcpServer::SendData(SOCKET sock, DataHeader* header)
{
	if (IsRun() && header)
	{
		return send(sock, (const char*)header, header->dataLength, 0);
	}

	return SOCKET_ERROR;
}

int EasyTcpServer::BatchSendData(DataHeader* header)
{
	if (IsRun() && header)
	{
		for (size_t i = 0; i < _clients.size(); ++i)
		{
			if (SendData(_clients[i]->sockfd(), header) == SOCKET_ERROR)
			{
				printf("<Socket=%d>发送数据失败\n", _clients[i]->sockfd());
			}
		}
	}
	return SOCKET_ERROR;
}

#endif
