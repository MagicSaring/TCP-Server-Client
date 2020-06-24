#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.LIB")

enum CMD
{
	CMD_LOGIN = 1,
	CMD_LOGIN_RESULT = 2,
	CMD_LOGOUT = 3,
	CMD_LOGOUT_RESULT = 4,
	CMD_ERROR = 5,
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

	while (true)
	{
		
		//3. ������������
		printf("��������������\n");
		char cmdBuff[128] = { 0 };
		scanf("%s", &cmdBuff);
		//4.������������
		if (0 == strcmp(cmdBuff, "exit"))
		{
			printf("�յ�exit����,�������\n");
			break;
		}
		else if(0 == strcmp(cmdBuff, "login"))
		{
			Login login;
			sprintf_s(login.userName, sizeof(login.userName), "magic");
			sprintf_s(login.passWord, sizeof(login.passWord), "rere2121");

			send(sock_client, (char*)&login, sizeof(login), 0);

			//���ܷ��������ص�����
			LoginResult loginRet = {};
			recv(sock_client, (char*)&loginRet, sizeof(loginRet), 0);

			printf("LoginResult:%d\n", loginRet.result);
		}
		else if (0 == strcmp(cmdBuff, "logout"))
		{
			Logout logout;
			sprintf_s(logout.userName, sizeof(logout.userName), "magic");
			send(sock_client, (char*)&logout, sizeof(logout), 0);

			//���ܷ��������ص�����
			LoginResult logoutRet = {};

			recv(sock_client, (char*)&logoutRet, sizeof(logoutRet), 0);

			printf("LogoutResult:%d\n", logoutRet.result);

		}
		else
		{
			printf("��֧�ֵ�����,����������.\n");
		}
	}

	//7.�ر��׽��� closesocket
	closesocket(sock_client);

	//8.���windows socket����
	WSACleanup();
	getchar();

	return 0;
}
