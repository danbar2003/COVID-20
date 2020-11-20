#pragma once
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "LocalNetwork.h"

class Communication
{
private:
	SOCKET udp_sock;
	SOCKET tcp_sock;
	LocalNetwork network;
	struct sockaddr_in master;

private:
	void SendReply(struct sockaddr_in& client);
public:
	Communication();

	void SyncRequest();
	void HandleIncomingsUDP();
	void HandleIncomingsTCP();
	void HandleCommandResults();
};
