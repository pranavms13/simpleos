/*
 * Network Definitions
 * Ethernet, IP, and other network structures
 */

#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>

/*==============================================================================
 * Ethernet Frame
 *============================================================================*/

#define ETH_ALEN        6       /* Ethernet address length */
#define ETH_HLEN        14      /* Ethernet header length */
#define ETH_MTU         1500    /* Maximum transmission unit */
#define ETH_FRAME_LEN   1514    /* Max frame length (header + MTU) */

/* Ethernet types */
#define ETH_TYPE_IP     0x0800
#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_IPV6   0x86DD

/* Ethernet header */
typedef struct {
    uint8_t  dest[ETH_ALEN];    /* Destination MAC */
    uint8_t  src[ETH_ALEN];     /* Source MAC */
    uint16_t type;              /* Protocol type (big-endian) */
} __attribute__((packed)) eth_header_t;

/* Broadcast MAC address */
static const uint8_t ETH_BROADCAST[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*==============================================================================
 * ARP (Address Resolution Protocol)
 *============================================================================*/

#define ARP_HW_ETHER    1       /* Ethernet hardware type */
#define ARP_OP_REQUEST  1       /* ARP request */
#define ARP_OP_REPLY    2       /* ARP reply */

typedef struct {
    uint16_t hw_type;           /* Hardware type */
    uint16_t proto_type;        /* Protocol type */
    uint8_t  hw_len;            /* Hardware address length */
    uint8_t  proto_len;         /* Protocol address length */
    uint16_t operation;         /* Operation */
    uint8_t  sender_mac[6];     /* Sender MAC */
    uint32_t sender_ip;         /* Sender IP */
    uint8_t  target_mac[6];     /* Target MAC */
    uint32_t target_ip;         /* Target IP */
} __attribute__((packed)) arp_packet_t;

/*==============================================================================
 * IPv4
 *============================================================================*/

#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17

typedef struct {
    uint8_t  version_ihl;       /* Version (4 bits) + IHL (4 bits) */
    uint8_t  tos;               /* Type of service */
    uint16_t total_len;         /* Total length */
    uint16_t id;                /* Identification */
    uint16_t flags_frag;        /* Flags (3 bits) + Fragment offset (13 bits) */
    uint8_t  ttl;               /* Time to live */
    uint8_t  protocol;          /* Protocol */
    uint16_t checksum;          /* Header checksum */
    uint32_t src_ip;            /* Source IP */
    uint32_t dest_ip;           /* Destination IP */
} __attribute__((packed)) ip_header_t;

/*==============================================================================
 * ICMP (Internet Control Message Protocol)
 *============================================================================*/

#define ICMP_ECHO_REPLY     0
#define ICMP_ECHO_REQUEST   8

typedef struct {
    uint8_t  type;              /* Message type */
    uint8_t  code;              /* Message code */
    uint16_t checksum;          /* Checksum */
    uint16_t id;                /* Identifier */
    uint16_t sequence;          /* Sequence number */
} __attribute__((packed)) icmp_header_t;

/*==============================================================================
 * UDP (User Datagram Protocol)
 *============================================================================*/

typedef struct {
    uint16_t src_port;          /* Source port */
    uint16_t dest_port;         /* Destination port */
    uint16_t length;            /* Length */
    uint16_t checksum;          /* Checksum */
} __attribute__((packed)) udp_header_t;

/*==============================================================================
 * Utility Functions
 *============================================================================*/

/* Byte order conversion (network is big-endian) */
static inline uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

static inline uint16_t ntohs(uint16_t val) {
    return htons(val);  /* Same operation */
}

static inline uint32_t htonl(uint32_t val) {
    return ((val & 0xFF) << 24) | 
           ((val & 0xFF00) << 8) |
           ((val >> 8) & 0xFF00) |
           ((val >> 24) & 0xFF);
}

static inline uint32_t ntohl(uint32_t val) {
    return htonl(val);  /* Same operation */
}

/* Make IP address from 4 octets */
#define MAKE_IP(a, b, c, d) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

/* Format MAC address to string */
static inline void mac_to_string(const uint8_t *mac, char *out) {
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 6; i++) {
        *out++ = hex[(mac[i] >> 4) & 0x0F];
        *out++ = hex[mac[i] & 0x0F];
        if (i < 5) *out++ = ':';
    }
    *out = '\0';
}

/* Format IP address to string */
static inline void ip_to_string(uint32_t ip, char *out) {
    for (int i = 0; i < 4; i++) {
        uint8_t octet = (ip >> (24 - i * 8)) & 0xFF;
        if (octet >= 100) { *out++ = '0' + octet / 100; octet %= 100; }
        if (octet >= 10) { *out++ = '0' + octet / 10; octet %= 10; }
        *out++ = '0' + octet;
        if (i < 3) *out++ = '.';
    }
    *out = '\0';
}

/* Calculate IP checksum */
static inline uint16_t ip_checksum(const void *data, int len) {
    const uint16_t *p = (const uint16_t *)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    
    if (len == 1) {
        sum += *(const uint8_t *)p;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

#endif /* _NET_H_ */
