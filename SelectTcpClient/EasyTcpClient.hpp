#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define FD_SETSIZE      2506
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

#ifndef RECV_BUFF_SIZE
#define  RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

class EasyTcpClient
{
public:
	EasyTcpClient()
	{
		m_hSocket = INVALID_SOCKET;
		memset(_szRecv, 0, sizeof(_szRecv));
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	//虚析构函数
	virtual ~EasyTcpClient()
	{
		Close();
	}

	//初始化socket
	int InitSocket()
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
			printf("<socket=%d>关闭旧连接..\n", m_hSocket);
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
		//printf("建立Socket=<%d>成功\n",m_hSocket);

		return 0;
	}	
	//连接服务器
	int Connect(const char* ip, short port)
	{
		if (INVALID_SOCKET == m_hSocket)
		{
			if (SOCKET_ERROR == InitSocket())
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
			printf("错误,Socket=<%d>连接服务器<ip:%s,port:%d>失败...\n", m_hSocket, ip, port);
			close(m_hSocket);
#endif // _WIN32
		}
		else
		{
			//printf("Socket=<%d>连接服务器<ip:%s,port:%d>成功...\n", m_hSocket, ip, port);
		}

		return ret;
	}
	//关闭socket
	void Close()
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
	int RecvData(SOCKET _sock)
	{
		// 5.接受客户端的请求数据
		int nLen = (int)recv(_sock, _szRecv, sizeof(_szRecv), 0);
		if (nLen <= 0)
		{
			printf("与服务端断开连接,客户端<Socket=%d>已退出,任务结束\n", (int)_sock);
			return -1;
		}

		//将收取到的数据拷贝到消息缓冲区
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);

		//消息缓冲区的数据尾部位置后移
		_lastPos += nLen;

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (_lastPos >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)_szMsgBuf;

			//判断消息缓冲区的数据长度大于消息长度
			if (_lastPos >= header->dataLength)
			{
				//消息缓冲区剩余未处理数据的长度
				int nSize = _lastPos - header->dataLength;

				//处理网络消息
				OnNetMsg(header);

				//将消息缓冲区剩余未处理数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);

				//消息缓冲区的数据尾部位置前移
				_lastPos = nSize;
			}
			else
			{
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}

		return 0;
	}
	//处理网络消息
	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult* loginRet = (LoginResult*)header;
			//printf("客户端<Socket=%d>收到服务端消息命令:CMD_LOGIN_RESULT, 数据长度:%d\n", (int)m_hSocket, loginRet->dataLength);
			//printf("LoginResult:%d\n", loginRet->result);
		}break;
		case CMD_LOGOUT_RESULT:
		{
			LogoutResult* logoutRet = (LogoutResult*)header;
			//printf("客户端<Socket=%d>收到服务端消息命令:CMD_LOGOUT_RESULT, 数据长度:%d\n", (int)m_hSocket, logoutRet->dataLength);
			//printf("LogoutResult:%d\n", logoutRet->result);
		}break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* userJoin = (NewUserJoin*)header;
			//printf("客户端<Socket=%d>收到服务端消息命令:CMD_NEW_USER_JOIN, 数据长度:%d\n", (int)m_hSocket, userJoin->dataLength);
			//printf("sock:%d\n", userJoin->sock);
		}break;
		case CMD_ERROR:
		{
			printf("客户端<Socket=%d>收到服务端消息命令:CMD_ERROR, 数据长度:%d\n", (int)m_hSocket, header->dataLength);
		}break;
		default:
		{
			printf("客户端<Socket=%d>收到未定义消息, 数据长度:%d\n", (int)m_hSocket, header->dataLength);
		}
		}
	}
	//发送数据
	int SendData(DataHeader* header, int nLen)
	{
		if (isRun() && header)
		{
			return send(m_hSocket, (const char*)header, nLen, 0);
		}
		return SOCKET_ERROR;
	}
	//处理网络消息
	bool OnRun()
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
			int ret = select(m_hSocket + 1, &fdReads, 0, 0, &t);
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

				if (-1 == RecvData(m_hSocket))
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
	//是否运行中
	bool isRun()
	{
		return m_hSocket != INVALID_SOCKET;
	}

private:
	SOCKET m_hSocket;

	//接收缓冲区
	char _szRecv[RECV_BUFF_SIZE] = {};

	//第二缓冲区,消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};

	//消息缓冲区的数据尾部位置
	int _lastPos = 0;
};


















#endif
