#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS


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

	Login()
	{
		memset(userName, 0, sizeof(userName));
		memset(passWord, 0, sizeof(passWord));
	}
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


	// -- ��Socket API��������TCP������
	//1.����һ��socket�׽���
	SOCKET sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_server == INVALID_SOCKET)
	{
		printf("����,����Socketʧ��...�������:%d", WSAGetLastError());
		WSACleanup();
		return 0;
	}

	//2.bind �����ڽ��ܿͻ������ӵ�����˿�
	struct sockaddr_in server_addr = {};
	int addr_len = sizeof(struct sockaddr_in);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4567);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (SOCKET_ERROR == bind(sock_server, (struct sockaddr*)&server_addr, addr_len))
	{
		printf("����,�����ڽ��ܿͻ������ӵ�����ӿ�ʧ��...�������:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
		return 0;
	}

	printf("������˿ڳɹ�\n");

	//3.listen ��������˿�
	if (SOCKET_ERROR == listen(sock_server, 5))
	{
		printf("����,��������˿�ʧ��...�������:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
	}

	printf("��������˿ڳɹ�...\n");

	//4.accept �ȴ����ܿͻ�������
	struct sockaddr_in client_addr = {};
	SOCKET new_socket = INVALID_SOCKET;
	//char sendBuff[128] = { 0 };

	new_socket = accept(sock_server, (struct sockaddr*)&client_addr, &addr_len);
	if (new_socket == INVALID_SOCKET)
	{
		printf("����,���ܵ���Ч�ͻ���Socket...�������:%d\n", WSAGetLastError());
		closesocket(sock_server);
		WSACleanup();
		return 0;
	}

	printf("�¿ͻ��˼���: Socket = %d, IP = %s\n", new_socket, inet_ntoa(client_addr.sin_addr));

	//char recvBuf[128] = {};
	while (true)
	{
		DataHeader header = {};
		// 5.���ܿͻ��˵���������
		int nLen = recv(new_socket, (char *)&header, sizeof(header), 0);
		if (nLen <= 0)
		{
			printf("�ͻ������˳�,�������\n");
			break;
		}

		printf("�յ�����:%d,���ݳ���;%d\n", header.cmd, header.dataLength);

		switch (header.cmd)
		{
			case CMD_LOGIN:
			{
				Login login = {};
				recv(new_socket, (char*)&login, sizeof(login), 0);

				printf("�û���¼...�û�����:%s, ����:%s\n", login.userName, login.passWord);
				
				LoginResult ret = {1};
				send(new_socket, (char*)&header, sizeof(header), 0);
				send(new_socket, (char*)&ret, sizeof(ret), 0);
			}break;
			case CMD_LOGOUT:
			{
				Logout logout = {};
				recv(new_socket, (char*)&logout, sizeof(logout), 0);

				printf("�û��ǳ�...�û�����:%s\n", logout.userName);

				LogoutResult ret = { 1 };
				
				send(new_socket, (char*)&header, sizeof(header), 0);
				send(new_socket, (char*)&ret, sizeof(ret), 0);
			}break;
			default:
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(new_socket, (char*)&header, sizeof(header), 0);
			break;
		}
		//memset(sendBuff, 0, sizeof(sendBuff));
		// 6.��������
		//if (0 == strcmp(recvBuf, "getInfo"))
		//{
		//	DataPackage dp = {66, "���"};
		//	send(new_socket, (char*)&dp, sizeof(dp), 0);
		//	
		//	/*sprintf_s(sendBuff, sizeof(sendBuff), "magic");
		//	send(new_socket, sendBuff, sizeof(sendBuff), 0);*/
		//}
		//else if (0 == strcmp(recvBuf, "getAge"))
		//{
		//	sprintf_s(sendBuff, sizeof(sendBuff), "66");
		//	send(new_socket, sendBuff, sizeof(sendBuff), 0);
		//}
		//else
		//{
		//	sprintf_s(sendBuff, sizeof(sendBuff), "??");
		//	send(new_socket, sendBuff, sizeof(sendBuff), 0);
		//}
	}

	//7.closesocket �ر��׽���
	closesocket(new_socket);
	closesocket(sock_server);

	//���Windows socket����
	WSACleanup();
	printf("���˳�,�������\n");
	getchar();

	return 0;
}