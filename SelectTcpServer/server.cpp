//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"
#include "MessageHeader.hpp"

int main()
{
	EasyTcpServer server;
	if (INVALID_SOCKET == server.InitSocket())
	{
		return 0;
	}

	if (SOCKET_ERROR == server.Bind(nullptr, PORT))
	{
		return 0;
	}

	if (SOCKET_ERROR == server.Listen(5))
	{
		return 0;
	}

	while (server.IsRun())
	{
		server.OnRun();
	}

	server.Close();

	printf("已退出,任务结束\n");

	return 0;
}