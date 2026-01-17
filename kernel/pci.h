/*
 * PCI Bus Driver
 * Enumerates and configures PCI devices
 */

#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>
#include <stddef.h>
#include "io.h"

/* PCI Configuration Space Ports */
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

/* PCI Configuration Space Registers */
#define PCI_VENDOR_ID        0x00
#define PCI_DEVICE_ID        0x02
#define PCI_COMMAND          0x04
#define PCI_STATUS           0x06
#define PCI_REVISION_ID      0x08
#define PCI_PROG_IF          0x09
#define PCI_SUBCLASS         0x0A
#define PCI_CLASS            0x0B
#define PCI_CACHE_LINE_SIZE  0x0C
#define PCI_LATENCY_TIMER    0x0D
#define PCI_HEADER_TYPE      0x0E
#define PCI_BIST             0x0F
#define PCI_BAR0             0x10
#define PCI_BAR1             0x14
#define PCI_BAR2             0x18
#define PCI_BAR3             0x1C
#define PCI_BAR4             0x20
#define PCI_BAR5             0x24
#define PCI_INTERRUPT_LINE   0x3C
#define PCI_INTERRUPT_PIN    0x3D

/* PCI Command Register Bits */
#define PCI_COMMAND_IO       0x0001
#define PCI_COMMAND_MEMORY   0x0002
#define PCI_COMMAND_MASTER   0x0004

/* PCI Class Codes */
#define PCI_CLASS_NETWORK    0x02
#define PCI_SUBCLASS_ETHERNET 0x00

/* Known Vendor/Device IDs */
#define PCI_VENDOR_INTEL     0x8086
#define PCI_VENDOR_REDHAT    0x1AF4  /* Red Hat (virtio) */
#define PCI_VENDOR_REALTEK   0x10EC

#define PCI_DEVICE_E1000     0x100E  /* Intel E1000 */
#define PCI_DEVICE_E1000E    0x10D3  /* Intel E1000E */
#define PCI_DEVICE_VIRTIO_NET 0x1000 /* Virtio network (legacy) */
#define PCI_DEVICE_RTL8139   0x8139  /* Realtek RTL8139 */

/* Maximum devices to scan */
#define PCI_MAX_DEVICES      32

/* PCI Device structure */
typedef struct {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint32_t bar[6];
    int      valid;
} pci_device_t;

/* Global device list */
static pci_device_t pci_devices[PCI_MAX_DEVICES];
static int pci_device_count = 0;

/*==============================================================================
 * PCI Configuration Space Access
 *============================================================================*/

/* Build PCI address for configuration access */
static inline uint32_t pci_address(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (1U << 31) |                    /* Enable bit */
           ((uint32_t)bus << 16) |
           ((uint32_t)(device & 0x1F) << 11) |
           ((uint32_t)(func & 0x07) << 8) |
           (offset & 0xFC);
}

/* Read 32-bit value from PCI config space */
static inline uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_address(bus, device, func, offset));
    return inl(PCI_CONFIG_DATA);
}

/* Read 16-bit value from PCI config space */
static inline uint16_t pci_read16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_address(bus, device, func, offset));
    return (uint16_t)(inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8));
}

/* Read 8-bit value from PCI config space */
static inline uint8_t pci_read8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_address(bus, device, func, offset));
    return (uint8_t)(inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8));
}

/* Write 32-bit value to PCI config space */
static inline void pci_write32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDR, pci_address(bus, device, func, offset));
    outl(PCI_CONFIG_DATA, value);
}

/* Write 16-bit value to PCI config space */
static inline void pci_write16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t value) {
    outl(PCI_CONFIG_ADDR, pci_address(bus, device, func, offset));
    uint32_t tmp = inl(PCI_CONFIG_DATA);
    tmp &= ~(0xFFFF << ((offset & 2) * 8));
    tmp |= (uint32_t)value << ((offset & 2) * 8);
    outl(PCI_CONFIG_DATA, tmp);
}

/*==============================================================================
 * PCI Device Enumeration
 *============================================================================*/

/* Check if device exists */
static inline int pci_device_exists(uint8_t bus, uint8_t device, uint8_t func) {
    uint16_t vendor = pci_read16(bus, device, func, PCI_VENDOR_ID);
    return vendor != 0xFFFF;
}

/* Get device info */
static void pci_get_device_info(uint8_t bus, uint8_t device, uint8_t func, pci_device_t *dev) {
    dev->bus = bus;
    dev->device = device;
    dev->function = func;
    dev->vendor_id = pci_read16(bus, device, func, PCI_VENDOR_ID);
    dev->device_id = pci_read16(bus, device, func, PCI_DEVICE_ID);
    dev->class_code = pci_read8(bus, device, func, PCI_CLASS);
    dev->subclass = pci_read8(bus, device, func, PCI_SUBCLASS);
    dev->prog_if = pci_read8(bus, device, func, PCI_PROG_IF);
    dev->revision = pci_read8(bus, device, func, PCI_REVISION_ID);
    dev->interrupt_line = pci_read8(bus, device, func, PCI_INTERRUPT_LINE);
    dev->interrupt_pin = pci_read8(bus, device, func, PCI_INTERRUPT_PIN);
    
    /* Read BARs */
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_read32(bus, device, func, PCI_BAR0 + i * 4);
    }
    
    dev->valid = 1;
}

/* Scan PCI buses - optimized for virtual machines */
static int pci_enumerate(void) {
    pci_device_count = 0;
    
    /* Only scan bus 0 for now - covers most VM devices */
    for (int bus = 0; bus < 1 && pci_device_count < PCI_MAX_DEVICES; bus++) {
        for (int device = 0; device < 32 && pci_device_count < PCI_MAX_DEVICES; device++) {
            /* Quick check: if device 0 function 0 doesn't exist, skip */
            if (!pci_device_exists(bus, device, 0)) {
                continue;
            }
            
            uint8_t header = pci_read8(bus, device, 0, PCI_HEADER_TYPE);
            int max_func = (header & 0x80) ? 8 : 1;
            
            for (int func = 0; func < max_func && pci_device_count < PCI_MAX_DEVICES; func++) {
                if (func > 0 && !pci_device_exists(bus, device, func)) {
                    continue;
                }
                
                pci_get_device_info(bus, device, func, &pci_devices[pci_device_count]);
                pci_device_count++;
            }
        }
    }
    
    return pci_device_count;
}

/* Find device by vendor/device ID */
static pci_device_t *pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id &&
            pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

/* Find device by class/subclass */
static pci_device_t *pci_find_class(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code &&
            pci_devices[i].subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

/* Enable bus mastering for device */
static void pci_enable_bus_master(pci_device_t *dev) {
    uint16_t cmd = pci_read16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    cmd |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY | PCI_COMMAND_IO;
    pci_write16(dev->bus, dev->device, dev->function, PCI_COMMAND, cmd);
}

/* Get BAR address (handles memory vs I/O BARs) */
static uint64_t pci_get_bar_address(pci_device_t *dev, int bar_num) {
    uint32_t bar = dev->bar[bar_num];
    
    if (bar & 0x01) {
        /* I/O BAR */
        return bar & 0xFFFFFFFC;
    } else {
        /* Memory BAR */
        uint32_t type = (bar >> 1) & 0x03;
        if (type == 0x02) {
            /* 64-bit BAR */
            uint64_t addr = (bar & 0xFFFFFFF0);
            addr |= ((uint64_t)dev->bar[bar_num + 1]) << 32;
            return addr;
        } else {
            /* 32-bit BAR */
            return bar & 0xFFFFFFF0;
        }
    }
}

#endif /* _PCI_H_ */
