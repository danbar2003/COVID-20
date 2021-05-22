#include "utils.h"

void build_packet(
	u_char* packet,
	u_long src_ip,
	u_long dst_ip,
	BYTE src_mac[ETH_ALEN],
	BYTE dst_mac[ETH_ALEN],
	short opcode
)
{
	ether_hdr* ethernet_header;
	arp_ether_ipv4* arp_header;

	ethernet_header = (ether_hdr*)packet;
	arp_header = (arp_ether_ipv4*)(packet + sizeof(ether_hdr));

	/* ethernet layer */
	memcpy(ethernet_header->src_addr, src_mac, ETH_ALEN);
	dst_mac ? memcpy(ethernet_header->dest_addr, dst_mac, ETH_ALEN)
		: memset(ethernet_header->dest_addr, 0xff, ETH_ALEN);
	ethernet_header->frame_type = htons(ETH_P_ARP);

	/* network layer */
	arp_header->htype = htons(ARP_HTYPE_ETHER);
	arp_header->ptype = htons(NETWORK_IPv4);
	arp_header->hlen = 6;
	arp_header->plen = 4;
	arp_header->op = htons(opcode); // who has
	memcpy(arp_header->sha, src_mac, ETH_ALEN);
	arp_header->spa = src_ip;
	dst_mac ? memcpy(arp_header->tha, dst_mac, ETH_ALEN)
		: memset(arp_header->tha, 0, ETH_ALEN);
	arp_header->tpa = dst_ip;
}

PIP_ADAPTER_INFO corresponding_adapter(
	pcap_if_t* pcap_adapter, 
	PIP_ADAPTER_INFO pAdapters
)
{
	const char* str1 = pcap_adapter->name;
	const char* str2;
	size_t i;

	str1 += strlen(str1) - 1;
	while (pAdapters)
	{
		i = 0;
		str2 = pAdapters->AdapterName;
		str2 += strlen(str2) - 1;

		while (*(str1 - i) == *(str2 - i) && *(str1 - i) != '{')
			i++;

		if (*(str1 - i) == *(str2 - i))
			return pAdapters;

		pAdapters = pAdapters->Next;
	}
	return NULL;
}

void v_adapter(pcap_if_t* alldevs, pcap_if_t** adapter_result, uint32_t v_ip, uint32_t* host, uint32_t* netmask)
{
	uint32_t lhost, lnetmask;

	for (; alldevs != NULL; alldevs = alldevs->next)
		for (pcap_addr_t* addr = alldevs->addresses; addr != NULL; addr = addr->next)
		{
			lhost = ((struct sockaddr_in*)(addr->addr))->sin_addr.s_addr;
			lnetmask = ((struct sockaddr_in*)(addr->netmask))->sin_addr.s_addr;

			if (lnetmask == 0)
				continue;

			if (lnetmask % 2 == 0)
				lnetmask = htonl(lnetmask);

			if ((lhost & lnetmask) == (v_ip & lnetmask))
			{
				*host = lhost;
				*netmask = lnetmask;
				*adapter_result = alldevs;
				return;
			}
		}
	*adapter_result = NULL;
}

uint32_t net_checksum_add(int len, uint8_t* buf)
{
	uint32_t sum = 0;
	int i;

	for (i = 0; i < len; i++) {
		if (i & 1)
			sum += (uint32_t)buf[i];
		else
			sum += (uint32_t)buf[i] << 8;
	}
	return sum;
}

uint16_t net_checksum_finish(uint32_t sum)
{
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	return ~sum;
}

uint16_t net_checksum_tcpudp(uint16_t length, uint16_t proto,
	uint8_t* addrs, uint8_t* buf)
{
	uint32_t sum = 0;

	sum += net_checksum_add(length, buf);         // payload
	sum += net_checksum_add(8, addrs);            // src + dst address
	sum += proto + length;                        // protocol & length
	return net_checksum_finish(sum);
}

uint16_t in_checksum(uint16_t* ptr, int nbytes)
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum = 0;
	while (nbytes > 1) {
		sum += *ptr++;
		nbytes -= 2;
	}
	if (nbytes == 1) {
		oddbyte = 0;
		*((u_char*)&oddbyte) = *(u_char*)ptr;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum = sum + (sum >> 16);
	answer = (SHORT)~sum;

	return answer;
}