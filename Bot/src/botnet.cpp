#include "botnet.h"

BotnetNode::BotnetNode()
	:_adr({ 0,0 })
{
}

BotnetNode::BotnetNode(uint32_t ip, uint16_t port)
	:_adr({ ip, port })
{
}

void BotnetNode::addPeer(uint32_t peer_ip, uint16_t port)
{
	_branches.push_back(BotnetNode(peer_ip, port));
}
