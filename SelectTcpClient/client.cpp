
#include "EasyTcpClient.hpp" 

void cmdThread(EasyTcpClient *client)
{
	while (true)
	{
		printf("Please input your cmd\n");

		char cmdBuff[256] = {};
		scanf("%s", &cmdBuff);
		if (0 == strcmp(cmdBuff, "exit"))
		{
			/*client->Close();*/
			printf("退出cmdThread线程\n");
			break;
		}
		else if (0 == strcmp(cmdBuff, "login"))
		{
			Login login;
			mystrcpy(login.userName, "magic");
			mystrcpy(login.passWord, "rere2121");

			client->SendData(&login);
		}

		else if (0 == strcmp(cmdBuff, "logout"))
		{
			Logout logout;
			mystrcpy(logout.userName, "magic");

			client->SendData(&logout);
		}
		else
		{
			printf("不支持的命令\n");
		}
	}
}

int main()
{
	EasyTcpClient client;
	if (client.Connect("127.0.0.1", 4567) == SOCKET_ERROR)
	{
		return 0;
	}

	EasyTcpClient client2;
	if (client2.Connect("192.168.40.99", 4567) == SOCKET_ERROR)
	{
		return 0;
	}

	EasyTcpClient client3;
	if (client3.Connect("192.168.40.99", 4568) == SOCKET_ERROR)
	{
		return 0;
	}

	//启动UI线程
	std::thread t1(cmdThread, &client);
	t1.detach();

	std::thread t2(cmdThread, &client2);
	t2.detach();

	std::thread t3(cmdThread, &client3);
	t3.detach();

	while (client.isRun() || client2.isRun() || client3.isRun())
	//while (client2.isRun())
	{
		client.OnRun();
		client2.OnRun();
		client3.OnRun();
	}
	
	client.Close();
	client2.Close();
	client3.Close();

	printf("已退出..\n");
	//getchar();

	return 0;
}
