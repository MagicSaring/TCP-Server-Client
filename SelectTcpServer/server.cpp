//#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;
#include <stdio.h>
#include <vector>
#include <algorithm>

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



enum CMD
{
	CMD_LOGIN = 1,
	CMD_LOGIN_RESULT = 2,
	CMD_LOGOUT = 3,
	CMD_LOGOUT_RESULT = 4,
	CMD_NEW_USER_JOIN = 5,
	CMD_ERROR = 6,
};
struct DataHeader
{
	short cmd;
	short dataLength;
};

//DataPackage
struct Login : public DataHeader
{
	Login()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(Login);
		memset(userName, 0, sizeof(userName));
		memset(passWord, 0, sizeof(passWord));
	}
	char userName[32];
	char passWord[32];
};

struct LoginResult : public DataHeader
{
	LoginResult()
	{
		cmd = CMD_LOGIN_RESULT;
		dataLength = sizeof(LoginResult);
		result = 0;
	}
	int result;
};

struct Logout : public DataHeader
{
	Logout()
	{
		cmd = CMD_LOGOUT;
		dataLength = sizeof(Logout);
		memset(userName, 0, sizeof(userName));
	}
	char userName[32];
};

struct LogoutResult : public DataHeader
{
	LogoutResult()
	{
		cmd = CMD_LOGOUT_RESULT;
		dataLength = sizeof(LogoutResult);
		result = 0;
	}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin()
	{
		cmd = CMD_NEW_USER_JOIN;
		dataLength = sizeof(NewUserJoin);
		sock = 0;
	}
	int sock;
};

vector<SOCKET> g_clients;

int dealReadWirte(SOCKET s)
{
	//缓冲区
	char szRecv[1024];
	// 5.接受客户端的请求数据
	int nLen = recv(s, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("客户端<Socket=%d>已退出,任务结束\n", s);
		return -1;
	}

	DataHeader* header = (DataHeader*)szRecv;

	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		Login* login = (Login*)szRecv;
		printf("收到客户端<Socket=%d>命令:CMD_LOGIN, 数据长度;%d\n", s, login->dataLength);
		printf("用户登录...用户姓名:%s, 密码:%s\n", login->userName, login->passWord);

		LoginResult ret;
		send(s, (char*)&ret, sizeof(ret), 0);
	}break;
	case CMD_LOGOUT:
	{
		//Logout logout;
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		Logout* logout = (Logout*)szRecv;
		printf("收到客户端<Socket=%d>命令:CMD_LOGOUT, 数据长度;%d\n", s, logout->dataLength);
		printf("用户登出...用户姓名:%s\n", logout->userName);

		LogoutResult ret;
		send(s, (char*)&ret, sizeof(ret), 0);
	}break;
	default:
		header->cmd = CMD_ERROR;
		header->dataLength = 0;
		send(s, (char*)&header, sizeof(header), 0);
		break;
	}
	
	return 0;
}

int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);		//生成版本号
	WSADATA data;

	//启动Windows socket 2.x环境
	if (WSAStartup(ver, &data) != 0)
	{
		printf("错误,加载winsock.dll失败...错误代码:%d", WSAGetLastError());
		return 0;
	}
#endif // _WIN32

	// -- 用Socket API建立简易TCP服务器
	//1.创建一个socket套接字
	SOCKET sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_server == INVALID_SOCKET)
	{
#ifdef _WIN32
		printf("错误,建立Socket失败...错误代码:%d", WSAGetLastError());
		WSACleanup();
#else
		printf("错误,建立Socket失败...\n");
#endif // _WIN32
		return 0;
	}

	//2.bind 绑定用于接受客户端连接的网络端口
	struct sockaddr_in server_addr = {};
	int addr_len = sizeof(struct sockaddr_in);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(6666);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (SOCKET_ERROR == bind(sock_server, (struct sockaddr*)&server_addr, addr_len))
	{
#ifdef _WIN32
		printf("错误,绑定用于接受客户端连接的网络接口失败...错误代码:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
#else
		printf("错误,绑定用于接受客户端连接的网络接口失败...\n");
		close(sock_server);
#endif // _WIN32

		return 0;
	}

	printf("绑定网络端口成功\n");

	//3.listen 监听网络端口
	if (SOCKET_ERROR == listen(sock_server, 5))
	{
#ifdef _WIN32
		printf("错误,监听网络端口失败...错误代码:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
#else
		printf("错误,监听网络端口失败...\n");
		close(sock_server);
#endif // _WIN32

		return 0;
	}

	printf("监听网络端口成功...\n");

	while (true)
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
		FD_SET(sock_server, &fdRead);
		FD_SET(sock_server, &fdWrite);
		FD_SET(sock_server, &fdExp);

		//SOCKET maxSocket = sock_server;

		for (size_t i = 0; i < g_clients.size(); ++i)
		{
			FD_SET(g_clients[i], &fdRead);
			//if (g_clients[i] > maxSocket)
			//{
			//	maxSocket = g_clients[i];
			//}
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
		//int ret = select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
		int ret = select(sock_server + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			printf("select任务结束\n");
			break;
		}

		//FD_ISSET:判断一个文件描述符是否在一个集合中，返回值:在1,不在0
		if (FD_ISSET(sock_server, &fdRead))
		{
			//FD_CLR:将一个文件描述符从集合中移除
			FD_CLR(sock_server, &fdRead);

			//4.accept 等待接受客户端连接
			struct sockaddr_in client_addr = {};
			SOCKET new_socket = INVALID_SOCKET;

#ifdef _WIN32
			new_socket = accept(sock_server, (struct sockaddr*)&client_addr, &addr_len);
#else
			new_socket = accept(sock_server, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
#endif // _WIN32

			
			if (new_socket == INVALID_SOCKET)
			{
#ifdef _WIN32
				printf("错误,接受到无效客户端Socket...错误代码:%d\n", WSAGetLastError());
				closesocket(new_socket);
#else		
				printf("错误,接受到无效客户端Socket\n");
				close(new_socket);
#endif // _WIN32
				continue;
			}

			for (size_t i = 0; i < g_clients.size(); ++i)
			{
				NewUserJoin userJoin;
				userJoin.sock = g_clients[i];
				send(g_clients[i], (char*)&userJoin, sizeof(userJoin), 0);
			}

			g_clients.push_back(new_socket);
			printf("新客户端加入: Socket = %d, IP = %s\n", new_socket, inet_ntoa(client_addr.sin_addr));
		}

		//for (size_t i = 0; i < g_clients.size(); ++i)
		//{
		//	if (FD_ISSET(g_clients[i], &fdRead))
		//	{
		//		if (-1 == dealReadWirte(g_clients[i]))
		//		{
		//			auto iter = find(g_clients.begin(), g_clients.end(), g_clients[i]);
		//			if (iter != g_clients.end())
		//			{
		//				g_clients.erase(iter);
		//			}
		//		}
		//	}
		//}

		for (size_t i = 0; i < fdRead.fd_count; ++i)
		{
			if (-1 == dealReadWirte(fdRead.fd_array[i]))
			{
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[i]);
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}
		
		//printf("空闲时间处理其他业务\n");
		
	}

#ifdef _WIN32
	//7.closesocket 关闭套接字
	for (size_t i = 0; i < g_clients.size(); ++i)
	{
		closesocket(g_clients[i]);
	}

	//清除Windows socket环境
	WSACleanup();
#else
	for (size_t i = 0; i < g_clients.size(); ++i)
	{
		close(g_clients[i]);
	}
#endif // _WIN32


	printf("已退出,任务结束\n");
	getchar();

	return 0;
}