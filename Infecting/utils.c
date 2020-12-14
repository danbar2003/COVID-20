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
	arp_header->ptype = htons(ARP_PTYPE_IPv4);
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