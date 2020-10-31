#pragma once

#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

enum PACK_TYPE
{
	//UDP
	SYNC_REQUEST, // Used when a bot wants to tell it exists
	SYNC_REPLY, // Used to send network structure syncs
	VERSION_SYNC_REQUEST, //Ask for database info 
	//TCP
	VERSION_SYNC //Sync database info
	,
};

struct botnet_pack
{
	enum PACK_TYPE type;
};

#ifdef __cplusplus
extern "C"
{
#endif


	extern const char LOCAL_IP[];
	extern const int MAX_IPSTR_BUF_SIZE; // 4 times xxx (255 fe) + 3 dots + null
	extern const size_t BOTNET_PACK_SIZE;
	extern const size_t BACKLOG;
	extern const INTERFACE_INFO InterfaceList[20];
	extern unsigned long DEC_MY_IP;

	int check(int exp, const char *msg);
	void init_winsock();
	void init_network_settings();
#ifdef __cplusplus
}
#endif