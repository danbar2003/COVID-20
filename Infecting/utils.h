#pragma once

#include <pcap/pcap.h>
#include <iphlpapi.h>

#include "network_headers.h"

void build_packet(
	u_char* packet, 
	u_long src_ip, 
	u_long dst_ip, 
	BYTE src_mac[ETH_ALEN], 
	BYTE dst_mac[ETH_ALEN],
	short opcode
);

PIP_ADAPTER_INFO corresponding_adapter(
	pcap_if_t* pcap_adapter, 
	PIP_ADAPTER_INFO pAdapters
);

uint32_t v_adapter(
	const pcap_if_t* alldevs, 
	pcap_if_t** adapter_result, 
	uint32_t v_ip
);
