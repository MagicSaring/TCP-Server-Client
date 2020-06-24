#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <vector>

using namespace std;

#pragma comment(lib, "WS2_32.LIB")

enum CMD
{
	CMD_LOGIN = 1,
	CMD_LOGIN_RESULT = 2,
	CMD_LOGOUT = 3,
	CMD_LOGOUT_RESULT = 4,
	CMD_ERROR = 5,
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

vector<SOCKET> g_clients;

int dealReadWirte(SOCKET s)
{
	//缓冲区
	char szRecv[1024];
	// 5.接受客户端的请求数据
	int nLen = recv(s, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("客户端已退出,任务结束\n");
		return -1;
	}

	DataHeader* header = (DataHeader*)szRecv;

	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		Login* login = (Login*)szRecv;
		printf("收到命令:CMD_LOGIN, 数据长度;%d\n", login->dataLength);
		printf("用户登录...用户姓名:%s, 密码:%s\n", login->userName, login->passWord);

		LoginResult ret;
		send(s, (char*)&ret, sizeof(ret), 0);
	}break;
	case CMD_LOGOUT:
	{
		//Logout logout;
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		Logout* logout = (Logout*)szRecv;
		printf("收到命令:CMD_LOGOUT, 数据长度;%d\n", logout->dataLength);
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
	WORD ver = MAKEWORD(2, 2);		//生成版本号
	WSADATA data;

	//启动Windows socket 2.x环境
	if (WSAStartup(ver, &data) != 0)
	{
		printf("错误,加载winsock.dll失败...错误代码:%d", WSAGetLastError());
		return 0;
	}

	// -- 用Socket API建立简易TCP服务器
	//1.创建一个socket套接字
	SOCKET sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_server == INVALID_SOCKET)
	{
		printf("错误,建立Socket失败...错误代码:%d", WSAGetLastError());
		WSACleanup();
		return 0;
	}

	//2.bind 绑定用于接受客户端连接的网络端口
	struct sockaddr_in server_addr = {};
	int addr_len = sizeof(struct sockaddr_in);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4567);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (SOCKET_ERROR == bind(sock_server, (struct sockaddr*)&server_addr, addr_len))
	{
		printf("错误,绑定用于接受客户端连接的网络接口失败...错误代码:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
		return 0;
	}

	printf("绑定网络端口成功\n");

	//3.listen 监听网络端口
	if (SOCKET_ERROR == listen(sock_server, 5))
	{
		printf("错误,监听网络端口失败...错误代码:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
	}

	printf("监听网络端口成功...\n");

	while (true)
	{
		//伯克利 socket
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(sock_server, &fdRead);
		FD_SET(sock_server, &fdWrite);
		FD_SET(sock_server, &fdExp);

		for (size_t i = 0; i < g_clients.size(); ++i)
		{
			FD_SET(g_clients[i], &fdRead);
		}

		//nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围,而不是数量
		//即是所有文件描述符最大值+1,在Windows中这个参数可以写0
		timeval t = { 0,0 };
		int ret = select(sock_server + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			printf("select任务结束\n");
			break;
		}

		if (FD_ISSET(sock_server, &fdRead))
		{
			FD_CLR(sock_server, &fdRead);

			//4.accept 等待接受客户端连接
			struct sockaddr_in client_addr = {};
			SOCKET new_socket = INVALID_SOCKET;
			new_socket = accept(sock_server, (struct sockaddr*)&client_addr, &addr_len);
			if (new_socket == INVALID_SOCKET)
			{
				printf("错误,接受到无效客户端Socket...错误代码:%d\n", WSAGetLastError());
				closesocket(new_socket);
				continue;
			}

			g_clients.push_back(new_socket);
			printf("新客户端加入: Socket = %d, IP = %s\n", new_socket, inet_ntoa(client_addr.sin_addr));
		}
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
	}

	//7.closesocket 关闭套接字
	for (size_t i = 0; i < g_clients.size(); ++i)
	{
		closesocket(g_clients[i]);
	}

	//清除Windows socket环境
	WSACleanup();
	printf("已退出,任务结束\n");
	getchar();

	return 0;
}