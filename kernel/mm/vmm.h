/*
 * Virtual Memory Manager (VMM)
 * 4-level paging (PML4 -> PDPT -> PD -> PT)
 */

#ifndef _VMM_H_
#define _VMM_H_

#include <stdint.h>
#include <stddef.h>

/* Page table entry flags */
#define VMM_PRESENT      (1ULL << 0)
#define VMM_WRITABLE     (1ULL << 1)
#define VMM_USER         (1ULL << 2)
#define VMM_PWT          (1ULL << 3)  // Page Write Through
#define VMM_PCD          (1ULL << 4)  // Page Cache Disable
#define VMM_ACCESSED     (1ULL << 5)
#define VMM_DIRTY        (1ULL << 6)
#define VMM_HUGE         (1ULL << 7)  // 2MB page
#define VMM_GLOBAL       (1ULL << 8)
#define VMM_NX           (1ULL << 63) // No Execute

/* Page table structures */
typedef struct {
    uint64_t entries[512];
} page_table_t;

typedef struct {
    uint64_t entries[512];
} page_directory_t;

typedef struct {
    uint64_t entries[512];
} page_directory_ptr_t;

typedef struct {
    uint64_t entries[512];
} pml4_t;

/* Function prototypes */
void vmm_init(void);
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(uint64_t virt);
uint64_t vmm_get_physical(uint64_t virt);
pml4_t *vmm_get_kernel_pml4(void);
pml4_t *vmm_create_address_space(void);
void vmm_switch_address_space(pml4_t *pml4);
void vmm_destroy_address_space(pml4_t *pml4);

/* Helper functions */
static inline void vmm_invalidate_tlb(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

static inline void vmm_reload_cr3(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

#endif /* _VMM_H_ */
