#pragma once

#include <stdint.h>

#define ACTS_NUM 2 //scan, infect

#define ETH_P_ARP 0x0806 /* Address Resolution packet */
#define ARP_HTYPE_ETHER 1  /* Ethernet ARP type */
#define ARP_PTYPE_IPv4 0x0800 /* Internet Protocol packet */
#define TRANSPORT_UDP 17 /* Transport Layer Protocol UDP */

#define ETH_ALEN 6 /* MAC Address bytes size */
#define MAX_HOSTS 100

#pragma pack(1)

typedef struct
{
    u_long ip;
    BYTE mac_addr[ETH_ALEN];
} host;

typedef struct
{
    pcap_t* fp;
    PIP_ADAPTER_INFO adapter;
    ULONG gateway_ip;
    BYTE gateway_mac[ETH_ALEN]; 
    ULONG victim_ip;
    BYTE victim_mac[ETH_ALEN];
} send_params;

/* Ethernet frame header */
typedef struct {
    uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
    uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
    uint16_t frame_type;    /* Ethernet frame type */
} ether_hdr;

/* Ethernet ARP packet from RFC 826 */
typedef struct {
    uint16_t htype;         /* Format of hardware address */
    uint16_t ptype;         /* Format of protocol address */
    uint8_t hlen;           /* Length of hardware address */
    uint8_t plen;           /* Length of protocol address */
    uint16_t op;            /* ARP opcode (command) */
    uint8_t sha[ETH_ALEN];  /* Sender hardware address */
    uint32_t spa;           /* Sender IP address */
    uint8_t tha[ETH_ALEN];  /* Target hardware address */
    uint32_t tpa;           /* Target IP address */
} arp_ether_ipv4;

 /* IP Header */
typedef struct
{
    uint8_t   ver_hlen;     /* Header version and length (dwords). */
    uint8_t   service;      /* Service type. */
    uint16_t  length;       /* Length of datagram (bytes). */
    uint16_t  ident;        /* Unique packet identification. */
    uint16_t  fragment;     /* Flags; Fragment offset. */
    uint8_t   timetolive;   /* Packet time to live (in network). */
    uint8_t   protocol;     /* Upper level protocol (UDP, TCP). */
    uint16_t  checksum;     /* IP header checksum. */
    uint32_t  src_addr;     /* Source IP address. */
    uint32_t  dest_addr;    /* Destination IP address. */
} ip_hdr;

/* UDP Header */
typedef struct
{
    uint16_t src_port;      /* Source port */
    uint16_t dst_port;      /* Destination port */
    uint16_t len;           /* Length */
    uint16_t chksum;        /* Checksum */
} udp_hdr;

/* DNS Header */
typedef struct
{
    uint16_t id;
    uint16_t flags;
    uint16_t questions;
    uint16_t answers;
    uint16_t authority;
    uint16_t additional;
} dns_hdr;