#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.LIB")

enum CMD
{
	CMD_LOGIN = 1,
	CMD_LOGOUT = 2,
	CMD_ERROR = 3,
};
struct DataHeader
{
	short cmd;
	short dataLength;
};

//DataPackage
struct Login
{
	char userName[32];
	char passWord[32];

	Login()
	{
		memset(userName, 0, sizeof(userName));
		memset(passWord, 0, sizeof(passWord));
	}
};

struct LoginResult
{
	int result;
};

struct Logout
{
	char userName[32];
};

struct LogoutResult
{
	int result;
};

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

	//4.accept 等待接受客户端连接
	struct sockaddr_in client_addr = {};
	SOCKET new_socket = INVALID_SOCKET;
	//char sendBuff[128] = { 0 };

	new_socket = accept(sock_server, (struct sockaddr*)&client_addr, &addr_len);
	if (new_socket == INVALID_SOCKET)
	{
		printf("错误,接受到无效客户端Socket...错误代码:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
		return 0;
	}

	printf("新客户端加入: Socket = %d, IP = %s\n", new_socket, inet_ntoa(client_addr.sin_addr));

	//char recvBuf[128] = {};
	while (true)
	{
		DataHeader header = {};
		// 5.接受客户端的请求数据
		int nLen = recv(new_socket, (char *)&header, sizeof(header), 0);
		if (nLen <= 0)
		{
			printf("客户端已退出,任务结束\n");
			break;
		}

		printf("收到命令:%d,数据长度;%d\n", header.cmd, header.dataLength);

		switch (header.cmd)
		{
			case CMD_LOGIN:
			{
				Login login = {};
				recv(new_socket, (char*)&login, sizeof(login), 0);

				printf("用户登录...用户姓名:%s, 密码:%s\n", login.userName, login.passWord);
				
				LoginResult ret = {1};
				send(new_socket, (char*)&header, sizeof(header), 0);
				send(new_socket, (char*)&ret, sizeof(ret), 0);
			}break;
			case CMD_LOGOUT:
			{
				Logout logout = {};
				recv(new_socket, (char*)&logout, sizeof(logout), 0);

				printf("用户登出...用户姓名:%s\n", logout.userName);

				LogoutResult ret = { 1 };
				
				send(new_socket, (char*)&header, sizeof(header), 0);
				send(new_socket, (char*)&ret, sizeof(ret), 0);
			}break;
			default:
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(new_socket, (char*)&header, sizeof(header), 0);
			break;
		}
		//memset(sendBuff, 0, sizeof(sendBuff));
		// 6.处理请求
		//if (0 == strcmp(recvBuf, "getInfo"))
		//{
		//	DataPackage dp = {66, "李宇春"};
		//	send(new_socket, (char*)&dp, sizeof(dp), 0);
		//	
		//	/*sprintf_s(sendBuff, sizeof(sendBuff), "magic");
		//	send(new_socket, sendBuff, sizeof(sendBuff), 0);*/
		//}
		//else if (0 == strcmp(recvBuf, "getAge"))
		//{
		//	sprintf_s(sendBuff, sizeof(sendBuff), "66");
		//	send(new_socket, sendBuff, sizeof(sendBuff), 0);
		//}
		//else
		//{
		//	sprintf_s(sendBuff, sizeof(sendBuff), "??");
		//	send(new_socket, sendBuff, sizeof(sendBuff), 0);
		//}
	}

	//7.closesocket 关闭套接字
	closesocket(new_socket);
	closesocket(sock_server);

	//清除Windows socket环境
	WSACleanup();
	printf("已退出,任务结束\n");
	getchar();

	return 0;
}