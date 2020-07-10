//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"
#include "MessageHeader.hpp"
#include <thread>

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
	EasyTcpServer server;
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

	server.Start();

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