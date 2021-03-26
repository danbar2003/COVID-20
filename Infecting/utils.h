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
	pcap_if_t* alldevs, 
	pcap_if_t** adapter_result, 
	uint32_t v_ip,
	uint32_t* host, 
	uint32_t* netmask
);

uint32_t net_checksum_add(
	int len,
	uint8_t* buf
);

uint16_t net_checksum_finish(
	uint32_t sum
);

uint16_t net_checksum_tcpudp(
	uint16_t length, 
	uint16_t proto,
	uint8_t* addrs, 
	uint8_t* buf
);

uint16_t in_checksum(
	uint16_t* ptr, 
	int nbytes
);