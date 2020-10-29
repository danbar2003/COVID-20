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

private:

public:
	Communication();
	~Communication();

	void SyncRequest();
	void HandleIncomings();
};
