#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h> //DEBUG
#include "network.h"

const char LOCAL_IP[] = "127.0.0.1";
const unsigned long DEC_LOCAL_IP = 16777343;
const int MAX_IPSTR_BUF_SIZE = 5 * 3 + 1; // 4 times xxx (255 fe) + 3 dots + null
const size_t BOTNET_PACK_SIZE = sizeof(struct botnet_pack);
const INTERFACE_INFO InterfaceList[20];

int check(int exp, const char* msg)
{
	if (exp == SOCKET_ERROR)
	{
		perror(msg);
		exit(1);
	}
	return exp;
}

int init_winsock()
{
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsock = WSAStartup(ver, &data);

	check(wsock, "init_winsock error");
}

int init_network_settings()
{

	SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	check(sd, "error init_network_settings building socket");

	int nBytesReturned;
	check(WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
		sizeof(InterfaceList), &nBytesReturned, 0, 0), "init_network_settings WSALoctl error ");
	
	closesocket(sd);
	return 0;
}