//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"
#include "MessageHeader.hpp"
#include <thread>

class MyServer : public EasyTcpServer
{
public:
	//只会被一个线程调用 安全
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		printf("client<%d> join\n",pClient->sockfd());
	}

	//cellServer 4 多个线程触发 不安全
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
		printf("client<%d> leave\n", pClient->sockfd());
	}

	//cellServer 4 多个线程触发 不安全 如果只开启一个cellServer就是安全的
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_recvCount++;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("收到客户端<Socket=%d>命令:CMD_LOGIN, 数据长度;%d\n", (int)sock, login->dataLength);
			//printf("用户登录...用户姓名:%s, 密码:%s\n", login->userName, login->passWord);

			LoginResult ret;
			pClient->SendData(&ret);
		}break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//printf("收到客户端<Socket=%d>命令:CMD_LOGOUT, 数据长度;%d\n", (int)sock, logout->dataLength);
			//printf("用户登出...用户姓名:%s\n", logout->userName);

			LogoutResult ret;
			pClient->SendData(&ret);
		}break;
		default:
		{
			printf("收到客户端<Socket=%d>未定义消息, 数据长度:%d\n", (int)pClient->sockfd(), header->dataLength);

			//DataHeader ret;
			//SendData(sock, &ret);
		}break;
		}
	}


private:

};



bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuff[256] = {};
		scanf("%s", &cmdBuff);
		if (0 == strcmp(cmdBuff, "exit"))
		{
			//->Close();
			g_bRun = false;
			printf("退出cmdThread线程\n");
			break;
		}
		else
		{
			printf("不支持的命令\n");
		}
	}
}

int main()
{
	MyServer server;
	if (INVALID_SOCKET == server.InitSocket())
	{
		return 0;
	}

	if (SOCKET_ERROR == server.Bind(nullptr, 4567))
	{
		return 0;
	}

	if (SOCKET_ERROR == server.Listen(5))
	{
		return 0;
	}

	server.Start(4);

	//启动UI线程
	std::thread t(cmdThread);
	t.detach();

	while (g_bRun)
	{
		server.OnRun();
	}

	server.Close();

	printf("已退出,任务结束\n");

	return 0;
}