#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h> //DEBUG
#include "network.h"

const char LOCAL_IP[] = "127.0.0.1";
const unsigned long DEC_LOCAL_IP = 16777343;
const int MAX_IPSTR_BUF_SIZE = 5 * 3 + 1; // 4 times xxx (255 fe) + 3 dots + null

const INTERFACE_INFO InterfaceList[20];

int init_winsock()
{
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsock = WSAStartup(ver, &data);

	if (wsock != 0)
		return 1; //TODO - modify it.
	return 0;
}

int init_network_settings()
{

	SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	if (sd == SOCKET_ERROR) {
		return 1;
	}

	int nBytesReturned;
	if (WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
		sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) {
		printf("%s %d\n", "Failed calling WSAIoctl: error ", WSAGetLastError());
		return 1;
	}
	closesocket(sd);
	return 0;
}