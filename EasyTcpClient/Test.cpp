#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.LIB")

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
		char cmdBuff[128] = { 0 };
		scanf("%s", &cmdBuff);
		//4.������������
		if (0 == strcmp(cmdBuff, "exit"))
		{
			printf("�յ�exit����,�������\n");
			break;
		}
		else
		{
			//5.���������������
			send(sock_client, cmdBuff, sizeof(cmdBuff), 0);
		}

		//6.���շ�������Ϣ recv
		char recvBuff[128] = { 0 };
		int nlen = recv(sock_client, recvBuff, sizeof(recvBuff), 0);
		if (nlen > 0)
		{
			printf("���յ�����:%s\n", recvBuff);
		}
	}

	//3. ���շ�������Ϣ recv
	char recvBuf[256] = {};
	int nlen = recv(sock_client, recvBuf, sizeof(recvBuf), 0);

	if (nlen > 0)
	{
		printf("���յ�����:%s", recvBuf);
	}

	//4.�ر��׽��� closesocket
	closesocket(sock_client);

	//5.���windows socket����
	WSACleanup();
	getchar();

	return 0;
}
