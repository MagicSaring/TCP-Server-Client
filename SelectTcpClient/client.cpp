
#include "EasyTcpClient.hpp"

bool g_bRun = true;

//客户端数量
const int cCount = 10;
//发送线程数量
const int tCount = 4;
//客户端数组
EasyTcpClient* client[cCount];

void sendThread(int id)
{
	printf("thread<%d>,start\n", id);
	//4个线程 1 - 4
	int c = cCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;

	for(int i = begin; i < end; ++i)
	{
		if (!g_bRun)
		{
			return;
		}
		client[i] = new EasyTcpClient();
	}

	for (int i = begin; i < end; ++i)
	{
		if (!g_bRun || client[i]->Connect("127.0.0.1", 4567) == SOCKET_ERROR)
		{
			return;
		}
	}

	printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

	std::chrono::milliseconds t(3000);
	std::this_thread::sleep_for(t);

	Login login[10];

	for (int i = 0; i < 10; ++i)
	{
		mystrcpy(login[i].userName, "magic");
		mystrcpy(login[i].passWord, "rere2121");
	}

	while (g_bRun)
	{
		for (int i = begin; i < end; ++i)
		{
			client[i]->SendData(login, 10);
			client[i]->OnRun();
		}
	}

	for (int i = begin; i < end; ++i)
	{
		client[i]->Close();
		delete client[i];
	}

	printf("thread<%d>,exit\n", id);
	//printf("已退出..\n");
}

void cmdThread()
{
	while (true)
	{
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
	//启动UI线程
	std::thread t(cmdThread);
	t.detach();

	//启动发送线程
	for (int i = 0; i < tCount; ++i)
	{
		std::thread t1(sendThread, i + 1);
		t1.detach();
	}

	while (g_bRun)
	{
		Sleep(100);
	}

	return 0;
}
