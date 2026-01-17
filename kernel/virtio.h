/*
 * Virtio Definitions
 * Common structures for virtio devices
 */

#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include <stdint.h>

/*==============================================================================
 * Virtio PCI Register Offsets (Legacy Interface)
 *============================================================================*/

#define VIRTIO_PCI_HOST_FEATURES     0x00  /* 32-bit: Features supported by host */
#define VIRTIO_PCI_GUEST_FEATURES    0x04  /* 32-bit: Features activated by guest */
#define VIRTIO_PCI_QUEUE_PFN         0x08  /* 32-bit: PFN for queue */
#define VIRTIO_PCI_QUEUE_SIZE        0x0C  /* 16-bit: Queue size */
#define VIRTIO_PCI_QUEUE_SEL         0x0E  /* 16-bit: Queue select */
#define VIRTIO_PCI_QUEUE_NOTIFY      0x10  /* 16-bit: Queue notify */
#define VIRTIO_PCI_STATUS            0x12  /* 8-bit: Device status */
#define VIRTIO_PCI_ISR               0x13  /* 8-bit: ISR status */
#define VIRTIO_PCI_CONFIG            0x14  /* Device-specific config */

/*==============================================================================
 * Virtio Device Status Bits
 *============================================================================*/

#define VIRTIO_STATUS_ACKNOWLEDGE    0x01
#define VIRTIO_STATUS_DRIVER         0x02
#define VIRTIO_STATUS_DRIVER_OK      0x04
#define VIRTIO_STATUS_FEATURES_OK    0x08
#define VIRTIO_STATUS_FAILED         0x80

/*==============================================================================
 * Virtio Feature Bits (Network)
 *============================================================================*/

#define VIRTIO_NET_F_CSUM            (1 << 0)   /* Host handles checksum */
#define VIRTIO_NET_F_GUEST_CSUM      (1 << 1)   /* Guest handles checksum */
#define VIRTIO_NET_F_MAC             (1 << 5)   /* Device has given MAC */
#define VIRTIO_NET_F_STATUS          (1 << 16)  /* Configuration status field */
#define VIRTIO_NET_F_MRG_RXBUF       (1 << 15)  /* Merge receive buffers */

/*==============================================================================
 * Virtqueue Structures
 *============================================================================*/

/* Virtqueue descriptor */
typedef struct {
    uint64_t addr;      /* Physical address of buffer */
    uint32_t len;       /* Length of buffer */
    uint16_t flags;     /* Flags */
    uint16_t next;      /* Next descriptor in chain */
} __attribute__((packed)) virtq_desc_t;

#define VIRTQ_DESC_F_NEXT     0x01  /* Buffer continues via next field */
#define VIRTQ_DESC_F_WRITE    0x02  /* Buffer is write-only (for device) */

/* Virtqueue available ring */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];    /* Array of descriptor indices */
} __attribute__((packed)) virtq_avail_t;

/* Virtqueue used element */
typedef struct {
    uint32_t id;        /* Descriptor chain head index */
    uint32_t len;       /* Bytes written */
} __attribute__((packed)) virtq_used_elem_t;

/* Virtqueue used ring */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    virtq_used_elem_t ring[];
} __attribute__((packed)) virtq_used_t;

/* Complete virtqueue structure */
typedef struct {
    uint16_t size;              /* Queue size */
    virtq_desc_t *desc;         /* Descriptor table */
    virtq_avail_t *avail;       /* Available ring */
    virtq_used_t *used;         /* Used ring */
    uint16_t free_head;         /* Head of free descriptor list */
    uint16_t num_free;          /* Number of free descriptors */
    uint16_t last_used_idx;     /* Last seen used index */
} virtqueue_t;

/*==============================================================================
 * Virtio Network Device Config
 *============================================================================*/

typedef struct {
    uint8_t mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
} __attribute__((packed)) virtio_net_config_t;

/*==============================================================================
 * Virtio Network Header (prepended to packets)
 *============================================================================*/

typedef struct {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;   /* Only if VIRTIO_NET_F_MRG_RXBUF */
} __attribute__((packed)) virtio_net_hdr_t;

#define VIRTIO_NET_HDR_GSO_NONE   0
#define VIRTIO_NET_HDR_GSO_TCPV4  1
#define VIRTIO_NET_HDR_GSO_UDP    3
#define VIRTIO_NET_HDR_GSO_TCPV6  4

#endif /* _VIRTIO_H_ */
