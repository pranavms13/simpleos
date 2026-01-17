/*
 * Boot Information Structure
 * Passed from UEFI bootloader to kernel
 */

#ifndef _BOOTINFO_H_
#define _BOOTINFO_H_

#include <stdint.h>

#define BOOTINFO_MAGIC 0x53494D504C454F53ULL  /* "SIMPLEOS" */

/* Memory map entry (matches UEFI format) */
typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t num_pages;
    uint64_t attribute;
} memory_map_entry_t;

/* Memory types */
#define MEMORY_RESERVED         0
#define MEMORY_LOADER_CODE      1
#define MEMORY_LOADER_DATA      2
#define MEMORY_BOOT_CODE        3
#define MEMORY_BOOT_DATA        4
#define MEMORY_RUNTIME_CODE     5
#define MEMORY_RUNTIME_DATA     6
#define MEMORY_CONVENTIONAL     7
#define MEMORY_UNUSABLE         8
#define MEMORY_ACPI_RECLAIM     9
#define MEMORY_ACPI_NVS         10
#define MEMORY_MMIO             11
#define MEMORY_MMIO_PORT        12

/* Framebuffer info */
typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;      /* Bytes per scanline */
    uint32_t bpp;        /* Bits per pixel */
    uint8_t  red_mask_size;
    uint8_t  red_mask_shift;
    uint8_t  green_mask_size;
    uint8_t  green_mask_shift;
    uint8_t  blue_mask_size;
    uint8_t  blue_mask_shift;
    uint8_t  reserved[2];
} framebuffer_info_t;

/* Boot information passed to kernel */
typedef struct {
    uint64_t magic;              /* BOOTINFO_MAGIC */
    uint64_t version;            /* Boot info version */
    
    /* Framebuffer */
    framebuffer_info_t framebuffer;
    
    /* Memory map */
    uint64_t mmap_entries;
    uint64_t mmap_entry_size;
    memory_map_entry_t *mmap;
    
    /* ACPI tables */
    uint64_t acpi_rsdp;
    
    /* Kernel info */
    uint64_t kernel_physical_base;
    uint64_t kernel_virtual_base;
    uint64_t kernel_size;
    
    /* Reserved for future use */
    uint64_t reserved[8];
} boot_info_t;

#endif /* _BOOTINFO_H_ */
