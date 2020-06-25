#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <thread>

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
	//������
	char szRecv[1024];
	// 5.���ܿͻ��˵���������
	int nLen = recv(s, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("�����˶Ͽ�����,�ͻ���<Socket=%d>���˳�,�������\n");
		return -1;
	}

	DataHeader* header = (DataHeader*)szRecv;

	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult* loginRet = (LoginResult*)szRecv;

		printf("�ͻ���<Socket=%d>�յ��������Ϣ����:CMD_LOGIN_RESULT, ���ݳ���:%d\n", s, loginRet->dataLength);
		printf("LoginResult:%d\n", loginRet->result);

	}break;
	case CMD_LOGOUT_RESULT:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult* logoutRet = (LogoutResult*)szRecv;

		printf("�ͻ���<Socket=%d>�յ��������Ϣ����:CMD_LOGOUT_RESULT, ���ݳ���:%d\n", s, logoutRet->dataLength);
		printf("LogoutResult:%d\n", logoutRet->result);

	}break;
	case CMD_NEW_USER_JOIN:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin* userJoin = (NewUserJoin*)szRecv;

		printf("�ͻ���<Socket=%d>�յ��������Ϣ����:CMD_NEW_USER_JOIN, ���ݳ���:%d\n", s, userJoin->dataLength);
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
			printf("�˳�cmdThread�߳�\n");
			break;
		}
		else if (0 == strcmp(cmdBuff, "login"))
		{
			Login login;
			strcpy_s(login.userName, sizeof(login.userName), "magic");
			strcpy_s(login.passWord, sizeof(login.passWord), "rere2121");
			send(s, (char*)&login, sizeof(login), 0);
		}

		else if (0 == strcmp(cmdBuff, "logout"))
		{
			Logout logout;
			strcpy_s(logout.userName, sizeof(logout.userName), "magic");
			send(s, (char*)&logout, sizeof(logout), 0);
		}
		else
		{
			printf("��֧�ֵ�����\n");
		}
	}
}

int main()
{
	WORD ver = MAKEWORD(2, 2);		//���ɰ汾��
	WSADATA data;

	//����Windows socket 2.x����
	if (WSAStartup(ver, &data) != 0)
	{
		printf("����,����winsock.dllʧ��...�������:%d", WSAGetLastError());
		return 0;
	}

	// -- ��Socket API��������TCP�ͻ���
	//1.����һ��socket�׽���
	SOCKET sock_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_client == INVALID_SOCKET)
	{
		printf("����,����Socketʧ��...�������:%d", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	printf("Socket�����ɹ�\n");
	//2. ���ӷ����� connect
	struct sockaddr_in server_addr; 
	int addr_len = sizeof(struct sockaddr_in);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4567);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int ret = connect(sock_client, (struct sockaddr*)&server_addr, addr_len);
	if (ret == SOCKET_ERROR)
	{
		printf("����,���ӷ�����ʧ��...�������:%d", WSAGetLastError());
		closesocket(sock_client);
		WSACleanup();
		return 0;
	}

	printf("���ӷ������ɹ�...\n");

	std::thread t1(cmdThread, sock_client);
	t1.detach();

	while (g_bRun)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);					//FD_ZERO:���һ���ļ�����������
		FD_SET(sock_client, &fdReads);		//FD_SET:���������ļ�����������ӵ�����������

		timeval t = { 1, 0 };
		int ret = select(sock_client + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			printf("�ͻ���<Socket=%d>select�������\n");
			break;
		}

		//FD_ISSET:�ж�һ���ļ��������Ƿ���һ�������У�����ֵ:��1,����0
		if (FD_ISSET(sock_client, &fdReads))
		{
			//FD_CLR:��һ���ļ��������Ӽ������Ƴ�
			FD_CLR(sock_client, &fdReads);

			if (-1 == dealReadWirte(sock_client))
			{
				printf("�ͻ���<Socket=%d>select�������\n");
				break;
			}
		}

		//printf("����ʱ�䴦������ҵ��\n");

	}

	//7.�ر��׽��� closesocket
	closesocket(sock_client);

	//8.���windows socket����
	WSACleanup();
	getchar();

	return 0;
}
