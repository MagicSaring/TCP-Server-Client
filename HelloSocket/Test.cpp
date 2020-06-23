#define  WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>

//#pragma comment(lib, "WS2_32.lib")

int main()
{
	WORD ver = MAKEWORD(2, 2);		//Éú³É°æ±¾ºÅ
	WSADATA dat;
	WSAStartup(ver, &dat);

	WSACleanup();
	return 0;
}