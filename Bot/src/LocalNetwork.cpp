#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "LocalNetwork.h"
#include "network.h"

//Private

//Public

LocalNetwork::LocalNetwork()
{
	ip_addr = InterfaceList[1].iiAddress.AddressIn.sin_addr.S_un.S_addr;
	next_hosts = std::vector<u_long>();
}

void LocalNetwork::AddHost(u_long host)
{
	if (std::find(next_hosts.begin(), next_hosts.end(), host) == next_hosts.end())
	{
		next_hosts.push_back(host);
	};
}

