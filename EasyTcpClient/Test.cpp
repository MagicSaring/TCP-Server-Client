#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.LIB")

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
		//3. 输入请求命令
		char cmdBuff[128] = { 0 };
		scanf("%s", &cmdBuff);
		//4.处理请求命令
		if (0 == strcmp(cmdBuff, "exit"))
		{
			printf("收到exit命令,任务结束\n");
			break;
		}
		else
		{
			//5.向服务器发送命令
			send(sock_client, cmdBuff, sizeof(cmdBuff), 0);
		}

		//6.接收服务器信息 recv
		char recvBuff[128] = { 0 };
		int nlen = recv(sock_client, recvBuff, sizeof(recvBuff), 0);
		if (nlen > 0)
		{
			printf("接收到数据:%s\n", recvBuff);
		}
	}

	//3. 接收服务器信息 recv
	char recvBuf[256] = {};
	int nlen = recv(sock_client, recvBuf, sizeof(recvBuf), 0);

	if (nlen > 0)
	{
		printf("接收到数据:%s", recvBuf);
	}

	//4.关闭套接字 closesocket
	closesocket(sock_client);

	//5.清楚windows socket环境
	WSACleanup();
	getchar();

	return 0;
}
