
//#define _WINSOCK_DEPRECATED_NO_WARNINGS

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
	int nLen = (int)recv(s, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("与服务端断开连接,客户端<Socket=%d>已退出,任务结束\n",s);
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

bool g_bRun = true;
void cmdThread(SOCKET s)
{
	while (true)
	{
		printf("Please input your cmd\n");
		char cmdBuff[256] = {};
		scanf("%s", &cmdBuff);
		if (0 == strcmp(cmdBuff, "exit"))
		{
			g_bRun = false;
			printf("退出cmdThread线程\n");
			break;
		}
		else if (0 == strcmp(cmdBuff, "login"))
		{
			Login login;
			mystrcpy(login.userName, "magic");
			mystrcpy(login.passWord, "rere2121");
			send(s, (char*)&login, sizeof(login), 0);
		}

		else if (0 == strcmp(cmdBuff, "logout"))
		{
			Logout logout;
			mystrcpy(logout.userName, "magic");
			send(s, (char*)&logout, sizeof(logout), 0);
		}
		else
		{
			printf("不支持的命令\n");
		}
	}
}

int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);		//生成版本号
	WSADATA data;

	//启动Windows socket 2.x环境
	if (WSAStartup(ver, &data) != 0)
	{
		printf("错误,加载winsock.dll失败...错误代码:%d\n", WSAGetLastError());
		return 0;
	}
#endif // _WIN32

	// -- 用Socket API建立简易TCP客户端
	//1.创建一个socket套接字
	SOCKET sock_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_client == INVALID_SOCKET)
	{
#ifdef _WIN32
		printf("错误,建立Socket失败...错误代码:%d\n", WSAGetLastError());
		WSACleanup();
#else
		printf("错误,建立Socket失败.....\n");
#endif
		return 0;
	}
	printf("Socket建立成功\n");
	//2. 连接服务器 connect
	struct sockaddr_in server_addr; 
	int addr_len = sizeof(struct sockaddr_in);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4567);
	server_addr.sin_addr.s_addr = inet_addr("192.168.40.99");

	int ret = connect(sock_client, (struct sockaddr*)&server_addr, addr_len);
	if (ret == SOCKET_ERROR)
	{
#ifdef _WIN32
		printf("错误,连接服务器失败...错误代码:%d\n", WSAGetLastError());
		closesocket(sock_client);
		WSACleanup();
#else
		printf("错误,连接服务器失败...\n");
		close(sock_client);
#endif // _WIN32
		return 0;
	}

	printf("连接服务器成功...\n");

	std::thread t1(cmdThread, sock_client);
	t1.detach();

	while (g_bRun)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);					//FD_ZERO:清空一个文件描述符集合
		FD_SET(sock_client, &fdReads);		//FD_SET:将监听的文件描述符，添加到监听集合中

		timeval t = { 1, 0 };
		int ret = select(sock_client + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			printf("客户端<Socket=%d>select任务结束\n", sock_client);
			break;
		}

		//FD_ISSET:判断一个文件描述符是否在一个集合中，返回值:在1,不在0
		if (FD_ISSET(sock_client, &fdReads))
		{
			//FD_CLR:将一个文件描述符从集合中移除
			FD_CLR(sock_client, &fdReads);

			if (-1 == dealReadWirte(sock_client))
			{
				printf("客户端<Socket=%d>select任务结束\n", sock_client);
				break;
			}
		}
		//printf("空闲时间处理其它业务\n");
	}

#ifdef _WIN32
	//7.关闭套接字 closesocket
	closesocket(sock_client);
	//8.清除windows socket环境
	WSACleanup();
#else
	//7.关闭套接字 closesocket
	close(sock_client);
#endif // _WIN32

	return 0;
}
