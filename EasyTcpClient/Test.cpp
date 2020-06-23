#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


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
			Login login = {"magic", "rere2121"};
			DataHeader dh = { CMD_LOGIN, sizeof(login) };
			
			send(sock_client, (char*)&dh, sizeof(dh), 0);
			send(sock_client, (char*)&login, sizeof(login), 0);

			//���ܷ��������ص�����
			DataHeader retHead = {};
			LoginResult loginRet = {};

			recv(sock_client, (char*)&retHead, sizeof(retHead), 0);
			recv(sock_client, (char*)&loginRet, sizeof(loginRet), 0);

			printf("LoginResult:%d\n", loginRet.result);
		}
		else if (0 == strcmp(cmdBuff, "logout"))
		{
			Login logout = { "magic"};
			DataHeader dh = { CMD_LOGOUT, sizeof(logout) };

			send(sock_client, (char*)&dh, sizeof(dh), 0);
			send(sock_client, (char*)&logout, sizeof(logout), 0);

			//���ܷ��������ص�����
			DataHeader retHead = {};
			LoginResult logoutRet = {};

			recv(sock_client, (char*)&retHead, sizeof(retHead), 0);
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
