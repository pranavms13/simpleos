/*
 * Physical Memory Manager Implementation
 * Uses bitmap allocator: 1 bit per 4KB page
 * 
 * Simplified version that limits memory to first 256MB for safety
 */

#include "pmm.h"
#include <stddef.h>

/* Maximum physical memory we'll manage (256MB) */
#define PMM_MAX_MEMORY      (256ULL * 1024 * 1024)
#define PMM_MAX_PAGES       (PMM_MAX_MEMORY / PAGE_SIZE)
#define PMM_BITMAP_SIZE     (PMM_MAX_PAGES / 8)

/* Static bitmap (in BSS, ~8KB for 256MB) */
static uint8_t bitmap[PMM_BITMAP_SIZE];
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;

/* Set bit in bitmap (mark page as used) */
static inline void bitmap_set(uint64_t page) {
    if (page < PMM_MAX_PAGES) {
        bitmap[page / 8] |= (1 << (page % 8));
    }
}

/* Clear bit in bitmap (mark page as free) */
static inline void bitmap_clear(uint64_t page) {
    if (page < PMM_MAX_PAGES) {
        bitmap[page / 8] &= ~(1 << (page % 8));
    }
}

/* Test bit in bitmap */
static inline int bitmap_test(uint64_t page) {
    if (page < PMM_MAX_PAGES) {
        return (bitmap[page / 8] >> (page % 8)) & 1;
    }
    return 1;  /* Out of range = used */
}

/* Initialize PMM from UEFI memory map */
void pmm_init(memory_map_entry_t *mmap, uint64_t entries) {
    /* Safety check */
    if (!mmap || entries == 0 || entries > 256) {
        total_pages = 0;
        used_pages = 0;
        return;
    }
    
    /* Start with all pages marked as used */
    for (uint64_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        bitmap[i] = 0xFF;
    }
    
    total_pages = PMM_MAX_PAGES;
    used_pages = PMM_MAX_PAGES;
    
    /* Mark free memory regions from the memory map */
    for (uint64_t i = 0; i < entries; i++) {
        /* Only process conventional/usable memory */
        if (mmap[i].type == MEMORY_CONVENTIONAL ||
            mmap[i].type == MEMORY_BOOT_CODE ||
            mmap[i].type == MEMORY_BOOT_DATA) {
            
            uint64_t start = mmap[i].physical_start;
            uint64_t num_pages = mmap[i].num_pages;
            
            /* Sanity check */
            if (num_pages > PMM_MAX_PAGES) {
                num_pages = PMM_MAX_PAGES;
            }
            
            uint64_t start_page = start / PAGE_SIZE;
            
            /* Mark these pages as free */
            for (uint64_t j = 0; j < num_pages && start_page + j < PMM_MAX_PAGES; j++) {
                if (bitmap_test(start_page + j)) {
                    bitmap_clear(start_page + j);
                    used_pages--;
                }
            }
        }
    }
    
    /* Re-mark first 2MB as used (kernel + reserved) */
    for (uint64_t page = 0; page < (2 * 1024 * 1024) / PAGE_SIZE; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            used_pages++;
        }
    }
}

/* Allocate a single page */
void *pmm_alloc_page(void) {
    /* Start searching after 2MB (where kernel is) */
    for (uint64_t page = (2 * 1024 * 1024) / PAGE_SIZE; page < PMM_MAX_PAGES; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            used_pages++;
            return (void*)(page * PAGE_SIZE);
        }
    }
    return NULL;
}

/* Free a single page */
void pmm_free_page(void *addr) {
    uint64_t page = ((uint64_t)(uintptr_t)addr) / PAGE_SIZE;
    if (page < PMM_MAX_PAGES && bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

/* Allocate contiguous pages */
void *pmm_alloc_pages(size_t count) {
    if (count == 0 || count > 256) return NULL;
    
    uint64_t start_search = (2 * 1024 * 1024) / PAGE_SIZE;
    
    for (uint64_t start_page = start_search; start_page + count <= PMM_MAX_PAGES; start_page++) {
        int found = 1;
        for (size_t i = 0; i < count; i++) {
            if (bitmap_test(start_page + i)) {
                found = 0;
                start_page += i;  /* Skip ahead */
                break;
            }
        }
        
        if (found) {
            for (size_t i = 0; i < count; i++) {
                bitmap_set(start_page + i);
                used_pages++;
            }
            return (void*)(start_page * PAGE_SIZE);
        }
    }
    
    return NULL;
}

/* Free contiguous pages */
void pmm_free_pages(void *addr, size_t count) {
    uint64_t start_page = ((uint64_t)(uintptr_t)addr) / PAGE_SIZE;
    
    for (size_t i = 0; i < count && start_page + i < PMM_MAX_PAGES; i++) {
        if (bitmap_test(start_page + i)) {
            bitmap_clear(start_page + i);
            used_pages--;
        }
    }
}

/* Get statistics */
uint64_t pmm_get_total_pages(void) {
    return total_pages;
}

uint64_t pmm_get_free_pages(void) {
    return total_pages > used_pages ? total_pages - used_pages : 0;
}

uint64_t pmm_get_used_pages(void) {
    return used_pages;
}
