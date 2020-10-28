#pragma once

#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#ifdef __cplusplus
extern "C"
{
#endif

	extern const char LOCAL_IP[];
	extern const unsigned long DEC_LOCAL_IP;
	extern const int MAX_IPSTR_BUF_SIZE; // 4 times xxx (255 fe) + 3 dots + null

	extern const INTERFACE_INFO InterfaceList[20];

	int init_winsock();
	int init_network_settings();
#ifdef __cplusplus
}
#endif