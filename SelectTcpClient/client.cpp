
#include "EasyTcpClient.hpp" 

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		/*printf("Please input your cmd\n");*/

		char cmdBuff[256] = {};
		scanf("%s", &cmdBuff);
		if (0 == strcmp(cmdBuff, "exit"))
		{
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
	//const int cCount = FD_SETSIZE - 1;
	const int cCount = 10000;
	EasyTcpClient *client[cCount];

	for (int i = 0; i < cCount; ++i)
	{
		if (!g_bRun)
		{
			return 0;
		}
		client[i] = new EasyTcpClient();
	}

	for (int i = 0; i < cCount; ++i)
	{
		if (!g_bRun || client[i]->Connect("127.0.0.1", 4567) == SOCKET_ERROR)
		{
			return 0;
		}

		printf("Connect=%d\n", i);
	}

	Login login;
	mystrcpy(login.userName, "magic");
	mystrcpy(login.passWord, "rere2121");

	//启动UI线程
	std::thread t(cmdThread);
	t.detach();

	while (g_bRun)
	{
		for (int i = 0; i < cCount; ++i)
		{
			client[i]->SendData(&login);
			client[i]->OnRun();
		}

	}

	for (int i = 0; i < cCount; ++i)
	{
		client[i]->Close();
	}

	printf("已退出..\n");

	return 0;
}
