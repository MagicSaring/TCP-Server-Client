#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <vector>

using namespace std;

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

vector<SOCKET> g_clients;

int dealReadWirte(SOCKET s)
{
	//������
	char szRecv[1024];
	// 5.���ܿͻ��˵���������
	int nLen = recv(s, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("�ͻ���<Socket=%d>���˳�,�������\n", s);
		return -1;
	}

	DataHeader* header = (DataHeader*)szRecv;

	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		Login* login = (Login*)szRecv;
		printf("�յ��ͻ���<Socket=%d>����:CMD_LOGIN, ���ݳ���;%d\n", s, login->dataLength);
		printf("�û���¼...�û�����:%s, ����:%s\n", login->userName, login->passWord);

		LoginResult ret;
		send(s, (char*)&ret, sizeof(ret), 0);
	}break;
	case CMD_LOGOUT:
	{
		//Logout logout;
		recv(s, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		Logout* logout = (Logout*)szRecv;
		printf("�յ��ͻ���<Socket=%d>����:CMD_LOGOUT, ���ݳ���;%d\n", s, logout->dataLength);
		printf("�û��ǳ�...�û�����:%s\n", logout->userName);

		LogoutResult ret;
		send(s, (char*)&ret, sizeof(ret), 0);
	}break;
	default:
		header->cmd = CMD_ERROR;
		header->dataLength = 0;
		send(s, (char*)&header, sizeof(header), 0);
		break;
	}
	
	return 0;
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

	while (true)
	{
		//������ socket
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		//FD_ZERO:���һ���ļ�����������
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		//FD_SET:���������ļ�����������ӵ�����������
		FD_SET(sock_server, &fdRead);
		FD_SET(sock_server, &fdWrite);
		FD_SET(sock_server, &fdExp);

		for (size_t i = 0; i < g_clients.size(); ++i)
		{
			FD_SET(g_clients[i], &fdRead);
		}

		/*
		select����ԭ��:
		int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);

		�����б�:
		nfds:		�����������ļ������������������ + 1(�ں˲�ȡ��ѯ�ķ�ʽ)
		readfds:	���ļ�������������
		writefds:	д�ļ�����������
		exceptfds:	�쳣�ļ�����������
		timeout:	����0:���ü�����ʱʱ��;NULL:��������;0:����������

		����ֵ:
		����0:��������������,�����Ӧʱ�������
		0:û�������
		-1:����error
		*/
		timeval t = { 1,0 };
		int ret = select(sock_server + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			printf("select�������\n");
			break;
		}

		//FD_ISSET:�ж�һ���ļ��������Ƿ���һ�������У�����ֵ:��1,����0
		if (FD_ISSET(sock_server, &fdRead))
		{
			//FD_CLR:��һ���ļ��������Ӽ������Ƴ�
			FD_CLR(sock_server, &fdRead);

			//4.accept �ȴ����ܿͻ�������
			struct sockaddr_in client_addr = {};
			SOCKET new_socket = INVALID_SOCKET;
			new_socket = accept(sock_server, (struct sockaddr*)&client_addr, &addr_len);
			if (new_socket == INVALID_SOCKET)
			{
				printf("����,���ܵ���Ч�ͻ���Socket...�������:%d\n", WSAGetLastError());
				closesocket(new_socket);
				continue;
			}

			for (size_t i = 0; i < g_clients.size(); ++i)
			{
				NewUserJoin userJoin;
				userJoin.sock = g_clients[i];
				send(g_clients[i], (char*)&userJoin, sizeof(userJoin), 0);
			}

			g_clients.push_back(new_socket);
			printf("�¿ͻ��˼���: Socket = %d, IP = %s\n", new_socket, inet_ntoa(client_addr.sin_addr));
		}
		for (size_t i = 0; i < fdRead.fd_count; ++i)
		{
			if (-1 == dealReadWirte(fdRead.fd_array[i]))
			{
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[i]);
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}
		
		//printf("����ʱ�䴦������ҵ��\n");
		
	}

	//7.closesocket �ر��׽���
	for (size_t i = 0; i < g_clients.size(); ++i)
	{
		closesocket(g_clients[i]);
	}

	//���Windows socket����
	WSACleanup();
	printf("���˳�,�������\n");
	getchar();

	return 0;
}