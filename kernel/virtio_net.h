/*
 * Virtio Network Driver
 * Implements a simple polling-based virtio-net driver for QEMU
 */

#ifndef _VIRTIO_NET_H_
#define _VIRTIO_NET_H_

#include <stdint.h>
#include "io.h"
#include "pci.h"
#include "virtio.h"
#include "net.h"

/*==============================================================================
 * Constants
 *============================================================================*/

#define VIRTIO_NET_QUEUE_RX     0
#define VIRTIO_NET_QUEUE_TX     1

#define VIRTIO_NET_RX_BUFFERS   16
#define VIRTIO_NET_TX_BUFFERS   16
#define VIRTIO_NET_BUFFER_SIZE  2048

/* Page size for virtqueue allocation */
#define PAGE_SIZE               4096

/*==============================================================================
 * Driver State
 *============================================================================*/

typedef struct {
    /* PCI device info */
    pci_device_t *pci_dev;
    uint16_t io_base;
    
    /* MAC address */
    uint8_t mac[6];
    
    /* Virtqueues */
    virtqueue_t rx_queue;
    virtqueue_t tx_queue;
    
    /* RX buffers */
    uint8_t rx_buffers[VIRTIO_NET_RX_BUFFERS][VIRTIO_NET_BUFFER_SIZE];
    
    /* TX buffers */
    uint8_t tx_buffers[VIRTIO_NET_TX_BUFFERS][VIRTIO_NET_BUFFER_SIZE];
    
    /* Statistics */
    uint64_t packets_rx;
    uint64_t packets_tx;
    uint64_t bytes_rx;
    uint64_t bytes_tx;
    
    /* Initialization status */
    int initialized;
} virtio_net_device_t;

/* Global device instance */
static virtio_net_device_t virtio_net_dev;

/* Static memory for virtqueues (must be page-aligned in real implementation) */
static uint8_t rx_queue_mem[PAGE_SIZE * 4] __attribute__((aligned(PAGE_SIZE)));
static uint8_t tx_queue_mem[PAGE_SIZE * 4] __attribute__((aligned(PAGE_SIZE)));

/*==============================================================================
 * Low-level I/O
 *============================================================================*/

static inline uint8_t virtio_read8(uint16_t base, uint16_t offset) {
    return inb(base + offset);
}

static inline uint16_t virtio_read16(uint16_t base, uint16_t offset) {
    return inw(base + offset);
}

static inline uint32_t virtio_read32(uint16_t base, uint16_t offset) {
    return inl(base + offset);
}

static inline void virtio_write8(uint16_t base, uint16_t offset, uint8_t val) {
    outb(base + offset, val);
}

static inline void virtio_write16(uint16_t base, uint16_t offset, uint16_t val) {
    outw(base + offset, val);
}

static inline void virtio_write32(uint16_t base, uint16_t offset, uint32_t val) {
    outl(base + offset, val);
}

/*==============================================================================
 * Virtqueue Management
 *============================================================================*/

/* Calculate virtqueue memory size */
static inline uint32_t virtq_size(uint16_t qsz) {
    return ((sizeof(virtq_desc_t) * qsz + sizeof(uint16_t) * (3 + qsz) + PAGE_SIZE - 1) 
            & ~(PAGE_SIZE - 1)) 
         + ((sizeof(uint16_t) * 3 + sizeof(virtq_used_elem_t) * qsz + PAGE_SIZE - 1) 
            & ~(PAGE_SIZE - 1));
}

/* Initialize a virtqueue */
static void virtq_init(virtqueue_t *vq, uint16_t size, void *mem) {
    vq->size = size;
    vq->desc = (virtq_desc_t *)mem;
    
    /* Available ring comes after descriptor table */
    vq->avail = (virtq_avail_t *)((uint8_t *)mem + sizeof(virtq_desc_t) * size);
    
    /* Used ring is page-aligned after available ring */
    uint32_t avail_size = sizeof(uint16_t) * (3 + size);
    uint32_t used_offset = ((sizeof(virtq_desc_t) * size + avail_size + PAGE_SIZE - 1) 
                            & ~(PAGE_SIZE - 1));
    vq->used = (virtq_used_t *)((uint8_t *)mem + used_offset);
    
    /* Initialize free list */
    for (int i = 0; i < size - 1; i++) {
        vq->desc[i].next = i + 1;
    }
    vq->free_head = 0;
    vq->num_free = size;
    vq->last_used_idx = 0;
    
    /* Clear available and used rings */
    vq->avail->flags = 0;
    vq->avail->idx = 0;
    vq->used->flags = 0;
    vq->used->idx = 0;
}

/* Add buffer to virtqueue */
static int virtq_add_buffer(virtqueue_t *vq, void *buf, uint32_t len, uint16_t flags) {
    if (vq->num_free == 0) {
        return -1;  /* No free descriptors */
    }
    
    uint16_t head = vq->free_head;
    vq->free_head = vq->desc[head].next;
    vq->num_free--;
    
    vq->desc[head].addr = (uint64_t)(uintptr_t)buf;
    vq->desc[head].len = len;
    vq->desc[head].flags = flags;
    vq->desc[head].next = 0;
    
    /* Add to available ring */
    uint16_t avail_idx = vq->avail->idx & (vq->size - 1);
    vq->avail->ring[avail_idx] = head;
    
    memory_barrier();
    vq->avail->idx++;
    memory_barrier();
    
    return (int)head;
}

/* Get used buffer from virtqueue */
static int virtq_get_used(virtqueue_t *vq, uint32_t *len) {
    if (vq->last_used_idx == vq->used->idx) {
        return -1;  /* No used buffers */
    }
    
    memory_barrier();
    
    uint16_t used_idx = vq->last_used_idx & (vq->size - 1);
    uint32_t desc_id = vq->used->ring[used_idx].id;
    *len = vq->used->ring[used_idx].len;
    
    vq->last_used_idx++;
    
    /* Return descriptor to free list */
    vq->desc[desc_id].next = vq->free_head;
    vq->free_head = desc_id;
    vq->num_free++;
    
    return (int)desc_id;
}

/*==============================================================================
 * Driver Initialization
 *============================================================================*/

/* Initialize the virtio-net driver (simplified - just detect and read MAC) */
static int virtio_net_init(void) {
    virtio_net_device_t *dev = &virtio_net_dev;
    
    /* Find virtio network device (PCI already scanned) */
    pci_device_t *pci = pci_find_device(PCI_VENDOR_REDHAT, PCI_DEVICE_VIRTIO_NET);
    if (!pci) {
        return -1;  /* No virtio-net device found */
    }
    
    dev->pci_dev = pci;
    
    /* Get I/O base address from BAR0 */
    uint32_t bar0 = pci->bar[0];
    if (!(bar0 & 0x01)) {
        return -2;  /* Not an I/O BAR */
    }
    dev->io_base = (uint16_t)(bar0 & 0xFFFC);
    
    if (dev->io_base == 0) {
        return -3;  /* Invalid I/O base */
    }
    
    /* Enable bus mastering */
    pci_enable_bus_master(pci);
    
    /* Reset device */
    virtio_write8(dev->io_base, VIRTIO_PCI_STATUS, 0);
    io_wait();
    
    /* Acknowledge device */
    virtio_write8(dev->io_base, VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    io_wait();
    
    /* Driver loaded */
    virtio_write8(dev->io_base, VIRTIO_PCI_STATUS, 
                  VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
    io_wait();
    
    /* Read device features */
    uint32_t features = virtio_read32(dev->io_base, VIRTIO_PCI_HOST_FEATURES);
    (void)features;  /* We'll accept any features for now */
    
    /* Write back that we accept the MAC feature */
    virtio_write32(dev->io_base, VIRTIO_PCI_GUEST_FEATURES, VIRTIO_NET_F_MAC);
    io_wait();
    
    /* Read MAC address from device config space */
    for (int i = 0; i < 6; i++) {
        dev->mac[i] = virtio_read8(dev->io_base, VIRTIO_PCI_CONFIG + i);
    }
    
    /* Mark driver OK (skip queue setup for now) */
    virtio_write8(dev->io_base, VIRTIO_PCI_STATUS,
                  VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);
    
    dev->initialized = 1;
    
    return 0;
}

/*==============================================================================
 * Packet Transmission
 *============================================================================*/

/* Send a raw ethernet frame */
static int virtio_net_send(const void *data, uint32_t len) {
    virtio_net_device_t *dev = &virtio_net_dev;
    
    if (!dev->initialized) {
        return -1;
    }
    
    if (len > ETH_FRAME_LEN) {
        return -2;  /* Packet too large */
    }
    
    if (dev->tx_queue.num_free < 2) {
        return -3;  /* No free buffers */
    }
    
    /* Find free TX buffer */
    static int tx_buf_idx = 0;
    uint8_t *buf = dev->tx_buffers[tx_buf_idx];
    tx_buf_idx = (tx_buf_idx + 1) % VIRTIO_NET_TX_BUFFERS;
    
    /* Prepare virtio-net header (12 bytes for simple mode) */
    virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)buf;
    hdr->flags = 0;
    hdr->gso_type = VIRTIO_NET_HDR_GSO_NONE;
    hdr->hdr_len = 0;
    hdr->gso_size = 0;
    hdr->csum_start = 0;
    hdr->csum_offset = 0;
    
    /* Copy packet data after header */
    uint8_t *pkt_data = buf + sizeof(virtio_net_hdr_t);
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < len; i++) {
        pkt_data[i] = src[i];
    }
    
    uint32_t total_len = sizeof(virtio_net_hdr_t) + len;
    
    /* Add to TX queue */
    virtq_add_buffer(&dev->tx_queue, buf, total_len, 0);
    
    /* Notify device */
    memory_barrier();
    virtio_write16(dev->io_base, VIRTIO_PCI_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_TX);
    
    dev->packets_tx++;
    dev->bytes_tx += len;
    
    return 0;
}

/*==============================================================================
 * Packet Reception
 *============================================================================*/

/* Receive a packet (polling) */
static int virtio_net_recv(void *buf, uint32_t buf_size) {
    virtio_net_device_t *dev = &virtio_net_dev;
    
    if (!dev->initialized) {
        return -1;
    }
    
    uint32_t len;
    int desc_id = virtq_get_used(&dev->rx_queue, &len);
    
    if (desc_id < 0) {
        return 0;  /* No packets */
    }
    
    /* Skip virtio-net header */
    uint8_t *rx_data = dev->rx_buffers[desc_id] + sizeof(virtio_net_hdr_t);
    uint32_t pkt_len = len - sizeof(virtio_net_hdr_t);
    
    if (pkt_len > buf_size) {
        pkt_len = buf_size;
    }
    
    /* Copy to user buffer */
    uint8_t *dst = (uint8_t *)buf;
    for (uint32_t i = 0; i < pkt_len; i++) {
        dst[i] = rx_data[i];
    }
    
    /* Re-add buffer to RX queue */
    virtq_add_buffer(&dev->rx_queue, dev->rx_buffers[desc_id], 
                     VIRTIO_NET_BUFFER_SIZE, VIRTQ_DESC_F_WRITE);
    virtio_write16(dev->io_base, VIRTIO_PCI_QUEUE_NOTIFY, VIRTIO_NET_QUEUE_RX);
    
    dev->packets_rx++;
    dev->bytes_rx += pkt_len;
    
    return (int)pkt_len;
}

/*==============================================================================
 * Utility Functions
 *============================================================================*/

/* Get MAC address */
static void virtio_net_get_mac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = virtio_net_dev.mac[i];
    }
}

/* Check if initialized */
static int virtio_net_is_ready(void) {
    return virtio_net_dev.initialized;
}

/* Get statistics */
static void virtio_net_get_stats(uint64_t *rx_packets, uint64_t *tx_packets,
                                  uint64_t *rx_bytes, uint64_t *tx_bytes) {
    if (rx_packets) *rx_packets = virtio_net_dev.packets_rx;
    if (tx_packets) *tx_packets = virtio_net_dev.packets_tx;
    if (rx_bytes) *rx_bytes = virtio_net_dev.bytes_rx;
    if (tx_bytes) *tx_bytes = virtio_net_dev.bytes_tx;
}

/*==============================================================================
 * High-Level Functions (ARP, Ping, etc.)
 *============================================================================*/

/* Our IP address (statically configured) */
static uint32_t our_ip = 0;

/* Set our IP address */
static void net_set_ip(uint32_t ip) {
    our_ip = ip;
}

/* Send ARP reply */
static void net_send_arp_reply(uint32_t target_ip, const uint8_t *target_mac) {
    uint8_t packet[ETH_HLEN + sizeof(arp_packet_t)];
    eth_header_t *eth = (eth_header_t *)packet;
    arp_packet_t *arp = (arp_packet_t *)(packet + ETH_HLEN);
    
    /* Ethernet header */
    for (int i = 0; i < 6; i++) {
        eth->dest[i] = target_mac[i];
        eth->src[i] = virtio_net_dev.mac[i];
    }
    eth->type = htons(ETH_TYPE_ARP);
    
    /* ARP reply */
    arp->hw_type = htons(ARP_HW_ETHER);
    arp->proto_type = htons(ETH_TYPE_IP);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->operation = htons(ARP_OP_REPLY);
    
    for (int i = 0; i < 6; i++) {
        arp->sender_mac[i] = virtio_net_dev.mac[i];
        arp->target_mac[i] = target_mac[i];
    }
    arp->sender_ip = htonl(our_ip);
    arp->target_ip = htonl(target_ip);
    
    virtio_net_send(packet, sizeof(packet));
}

/* Send ICMP echo reply (ping response) */
static void net_send_ping_reply(uint32_t dest_ip, const uint8_t *dest_mac,
                                 uint16_t id, uint16_t seq,
                                 const void *data, uint32_t data_len) {
    uint8_t packet[ETH_FRAME_LEN];
    eth_header_t *eth = (eth_header_t *)packet;
    ip_header_t *ip = (ip_header_t *)(packet + ETH_HLEN);
    icmp_header_t *icmp = (icmp_header_t *)((uint8_t *)ip + 20);
    
    /* Ethernet header */
    for (int i = 0; i < 6; i++) {
        eth->dest[i] = dest_mac[i];
        eth->src[i] = virtio_net_dev.mac[i];
    }
    eth->type = htons(ETH_TYPE_IP);
    
    /* IP header */
    uint16_t total_len = 20 + 8 + data_len;  /* IP + ICMP header + data */
    ip->version_ihl = 0x45;  /* IPv4, 5 dwords header */
    ip->tos = 0;
    ip->total_len = htons(total_len);
    ip->id = 0;
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IP_PROTO_ICMP;
    ip->checksum = 0;
    ip->src_ip = htonl(our_ip);
    ip->dest_ip = htonl(dest_ip);
    ip->checksum = ip_checksum(ip, 20);
    
    /* ICMP header */
    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = id;
    icmp->sequence = seq;
    
    /* Copy payload */
    uint8_t *payload = (uint8_t *)icmp + 8;
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < data_len; i++) {
        payload[i] = src[i];
    }
    
    /* Calculate ICMP checksum */
    icmp->checksum = ip_checksum(icmp, 8 + data_len);
    
    virtio_net_send(packet, ETH_HLEN + total_len);
}

/* Process received packet */
static void net_process_packet(const uint8_t *packet, uint32_t len) {
    if (len < ETH_HLEN) return;
    
    const eth_header_t *eth = (const eth_header_t *)packet;
    uint16_t eth_type = ntohs(eth->type);
    
    if (eth_type == ETH_TYPE_ARP) {
        /* Handle ARP */
        if (len < ETH_HLEN + sizeof(arp_packet_t)) return;
        
        const arp_packet_t *arp = (const arp_packet_t *)(packet + ETH_HLEN);
        if (ntohs(arp->operation) == ARP_OP_REQUEST) {
            uint32_t target_ip = ntohl(arp->target_ip);
            if (target_ip == our_ip) {
                net_send_arp_reply(ntohl(arp->sender_ip), arp->sender_mac);
            }
        }
    }
    else if (eth_type == ETH_TYPE_IP) {
        /* Handle IP */
        if (len < ETH_HLEN + 20) return;
        
        const ip_header_t *ip = (const ip_header_t *)(packet + ETH_HLEN);
        if ((ip->version_ihl >> 4) != 4) return;  /* IPv4 only */
        
        uint32_t dest_ip = ntohl(ip->dest_ip);
        if (dest_ip != our_ip && dest_ip != 0xFFFFFFFF) return;  /* Not for us */
        
        if (ip->protocol == IP_PROTO_ICMP) {
            /* Handle ICMP */
            uint32_t ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
            const icmp_header_t *icmp = (const icmp_header_t *)((const uint8_t *)ip + ip_hdr_len);
            
            if (icmp->type == ICMP_ECHO_REQUEST) {
                /* Ping request - send reply */
                uint32_t icmp_len = ntohs(ip->total_len) - ip_hdr_len;
                uint32_t data_len = icmp_len > 8 ? icmp_len - 8 : 0;
                
                net_send_ping_reply(
                    ntohl(ip->src_ip),
                    eth->src,
                    icmp->id,
                    icmp->sequence,
                    (const uint8_t *)icmp + 8,
                    data_len
                );
            }
        }
    }
}

/* Poll for incoming packets and process them */
static void net_poll(void) {
    uint8_t buffer[ETH_FRAME_LEN];
    int len;
    
    while ((len = virtio_net_recv(buffer, sizeof(buffer))) > 0) {
        net_process_packet(buffer, len);
    }
}

#endif /* _VIRTIO_NET_H_ */
