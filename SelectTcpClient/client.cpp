#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.LIB")

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

int dealReadWirte(SOCKET s)
{
	//缓冲区
	char szRecv[1024];
	// 5.接受客户端的请求数据
	int nLen = recv(s, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("与服务端断开连接,客户端<Socket=%d>已退出,任务结束\n");
		return -1;
	}

	DataHeader* header = (DataHeader*)szRecv;

	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult* loginRet = (LoginResult*)szRecv;

		printf("客户端<Socket=%d>收到服务端消息命令:CMD_LOGIN_RESULT, 数据长度:%d\n", s, loginRet->dataLength);
		printf("LoginResult:%d\n", loginRet->result);

	}break;
	case CMD_LOGOUT_RESULT:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult* logoutRet = (LogoutResult*)szRecv;

		printf("客户端<Socket=%d>收到服务端消息命令:CMD_LOGOUT_RESULT, 数据长度:%d\n", s, logoutRet->dataLength);
		printf("LogoutResult:%d\n", logoutRet->result);

	}break;
	case CMD_NEW_USER_JOIN:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin* userJoin = (NewUserJoin*)szRecv;

		printf("客户端<Socket=%d>收到服务端消息命令:CMD_NEW_USER_JOIN, 数据长度:%d\n", s, userJoin->dataLength);
		printf("sock:%d\n", userJoin->sock);
	}break;
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

	// -- 用Socket API建立简易TCP客户端
	//1.创建一个socket套接字
	SOCKET sock_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_client == INVALID_SOCKET)
	{
		printf("错误,建立Socket失败...错误代码:%d", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	printf("Socket建立成功\n");
	//2. 连接服务器 connect
	struct sockaddr_in server_addr; 
	int addr_len = sizeof(struct sockaddr_in);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4567);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int ret = connect(sock_client, (struct sockaddr*)&server_addr, addr_len);
	if (ret == SOCKET_ERROR)
	{
		printf("错误,连接服务器失败...错误代码:%d", WSAGetLastError());
		closesocket(sock_client);
		WSACleanup();
		return 0;
	}

	printf("连接服务器成功...\n");

	while (true)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(sock_client, &fdReads);

		timeval t = { 0, 0 };
		int ret = select(sock_client + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			printf("客户端<Socket=%d>select任务结束\n");
			break;
		}

		if (FD_ISSET(sock_client, &fdReads))
		{
			FD_CLR(sock_client, &fdReads);

			if (-1 == dealReadWirte(sock_client))
			{
				printf("客户端<Socket=%d>select任务结束\n");
				break;
			}
		}

		printf("空闲时间处理其它业务\n");
		Login login;
		strcpy_s(login.userName, sizeof(login.userName), "magic");
		strcpy_s(login.passWord, sizeof(login.passWord), "rere2121");
		send(sock_client, (char *)&login, sizeof(login), 0);
		Sleep(1000);
	}

	//7.关闭套接字 closesocket
	closesocket(sock_client);

	//8.清除windows socket环境
	WSACleanup();
	getchar();

	return 0;
}
