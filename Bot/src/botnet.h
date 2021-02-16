#pragma once

#include <vector>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

class BotnetNode
{
	struct
	{
		uint32_t _ip;
		uint16_t _port;
	} _adr;

	std::vector<BotnetNode> _branches;

public:
	BotnetNode();

	BotnetNode(uint32_t ip, uint16_t port);

	void addPeer(uint32_t peer_ip, uint16_t peer_port);
};
