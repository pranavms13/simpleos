/*
 * Physical Memory Manager (PMM)
 * Bitmap-based page allocator (4KB pages)
 */

#ifndef _PMM_H_
#define _PMM_H_

#include <stdint.h>
#include <stddef.h>
#include "../../include/bootinfo.h"

/* Page size */
#define PAGE_SIZE       4096
#define PAGE_SIZE_2MB   2097152

/* Function prototypes */
void pmm_init(memory_map_entry_t *mmap, uint64_t entries);
void *pmm_alloc_page(void);
void pmm_free_page(void *addr);
void *pmm_alloc_pages(size_t count);
void pmm_free_pages(void *addr, size_t count);
uint64_t pmm_get_total_pages(void);
uint64_t pmm_get_free_pages(void);
uint64_t pmm_get_used_pages(void);

#endif /* _PMM_H_ */
