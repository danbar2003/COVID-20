#pragma once
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "NetworkTree.h"

class Communication
{
private:
	SOCKET udp_sock;
	SOCKET tcp_sock;
	NetworkTree* NetTree;
public:
	Communication();
	~Communication();

};
