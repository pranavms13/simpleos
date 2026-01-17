/*
 * Virtual Memory Manager Implementation
 * 4-level paging for x86_64
 */

#include "vmm.h"
#include "pmm.h"
#include <stddef.h>

/* Kernel PML4 (page map level 4) */
static pml4_t *kernel_pml4 = NULL;

/* Get current CR3 (PML4 physical address) */
static inline uint64_t vmm_get_cr3(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

/* Set CR3 */
static inline void vmm_set_cr3(uint64_t cr3) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

/* Get page table entry index from virtual address */
static inline uint64_t pml4_index(uint64_t virt) {
    return (virt >> 39) & 0x1FF;
}

static inline uint64_t pdpt_index(uint64_t virt) {
    return (virt >> 30) & 0x1FF;
}

static inline uint64_t pd_index(uint64_t virt) {
    return (virt >> 21) & 0x1FF;
}

static inline uint64_t pt_index(uint64_t virt) {
    return (virt >> 12) & 0x1FF;
}

/* Allocate a page table (physical page) */
static uint64_t vmm_alloc_page_table(void) {
    void *page = pmm_alloc_page();
    if (!page) return 0;
    
    /* Clear the page */
    uint64_t *entries = (uint64_t*)page;
    for (int i = 0; i < 512; i++) {
        entries[i] = 0;
    }
    
    return (uint64_t)(uintptr_t)page;
}

/* Get or create page table entry */
static uint64_t *vmm_get_or_create_entry(uint64_t virt, uint64_t flags __attribute__((unused))) {
    pml4_t *pml4 = kernel_pml4;
    if (!pml4) return NULL;
    
    uint64_t pml4_idx = pml4_index(virt);
    uint64_t pml4_entry = pml4->entries[pml4_idx];
    
    page_directory_ptr_t *pdpt;
    if (!(pml4_entry & VMM_PRESENT)) {
        uint64_t pdpt_phys = vmm_alloc_page_table();
        if (!pdpt_phys) return NULL;
        
        pml4_entry = pdpt_phys | VMM_PRESENT | VMM_WRITABLE;
        pml4->entries[pml4_idx] = pml4_entry;
        
        /* Map PDPT to virtual address (identity map for now) */
        pdpt = (page_directory_ptr_t*)(uintptr_t)pdpt_phys;
    } else {
        uint64_t pdpt_phys = pml4_entry & ~0xFFF;
        pdpt = (page_directory_ptr_t*)(uintptr_t)pdpt_phys;
    }
    
    uint64_t pdpt_idx = pdpt_index(virt);
    uint64_t pdpt_entry = pdpt->entries[pdpt_idx];
    
    page_directory_t *pd;
    if (!(pdpt_entry & VMM_PRESENT)) {
        uint64_t pd_phys = vmm_alloc_page_table();
        if (!pd_phys) return NULL;
        
        pdpt_entry = pd_phys | VMM_PRESENT | VMM_WRITABLE;
        pdpt->entries[pdpt_idx] = pdpt_entry;
        
        pd = (page_directory_t*)(uintptr_t)pd_phys;
    } else {
        uint64_t pd_phys = pdpt_entry & ~0xFFF;
        pd = (page_directory_t*)(uintptr_t)pd_phys;
    }
    
    uint64_t pd_idx = pd_index(virt);
    uint64_t pd_entry = pd->entries[pd_idx];
    
    page_table_t *pt;
    if (!(pd_entry & VMM_PRESENT)) {
        uint64_t pt_phys = vmm_alloc_page_table();
        if (!pt_phys) return NULL;
        
        pd_entry = pt_phys | VMM_PRESENT | VMM_WRITABLE;
        pd->entries[pd_idx] = pd_entry;
        
        pt = (page_table_t*)(uintptr_t)pt_phys;
    } else {
        uint64_t pt_phys = pd_entry & ~0xFFF;
        pt = (page_table_t*)(uintptr_t)pt_phys;
    }
    
    uint64_t pt_idx = pt_index(virt);
    return &pt->entries[pt_idx];
}

/* Initialize VMM */
void vmm_init(void) {
    /* Get current PML4 from CR3 */
    uint64_t cr3 = vmm_get_cr3();
    kernel_pml4 = (pml4_t*)(uintptr_t)cr3;
    
    if (!kernel_pml4) {
        /* Allocate new PML4 if needed */
        void *pml4_page = pmm_alloc_page();
        if (!pml4_page) {
            /* Out of memory - halt */
            while (1) {
                __asm__ volatile("hlt");
            }
        }
        
        kernel_pml4 = (pml4_t*)pml4_page;
        
        /* Clear PML4 */
        for (int i = 0; i < 512; i++) {
            kernel_pml4->entries[i] = 0;
        }
        
        /* Set CR3 */
        vmm_set_cr3((uint64_t)(uintptr_t)kernel_pml4);
    }
}

/* Map a virtual page to a physical page */
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *entry = vmm_get_or_create_entry(virt, flags);
    if (!entry) return;
    
    *entry = (phys & ~0xFFF) | flags | VMM_PRESENT;
    vmm_invalidate_tlb(virt);
}

/* Unmap a virtual page */
void vmm_unmap_page(uint64_t virt) {
    uint64_t *entry = vmm_get_or_create_entry(virt, 0);
    if (!entry) return;
    
    *entry = 0;
    vmm_invalidate_tlb(virt);
}

/* Get physical address from virtual address */
uint64_t vmm_get_physical(uint64_t virt) {
    pml4_t *pml4 = kernel_pml4;
    if (!pml4) return 0;
    
    uint64_t pml4_idx = pml4_index(virt);
    uint64_t pml4_entry = pml4->entries[pml4_idx];
    if (!(pml4_entry & VMM_PRESENT)) return 0;
    
    page_directory_ptr_t *pdpt = (page_directory_ptr_t*)(uintptr_t)(pml4_entry & ~0xFFF);
    uint64_t pdpt_idx = pdpt_index(virt);
    uint64_t pdpt_entry = pdpt->entries[pdpt_idx];
    if (!(pdpt_entry & VMM_PRESENT)) return 0;
    
    page_directory_t *pd = (page_directory_t*)(uintptr_t)(pdpt_entry & ~0xFFF);
    uint64_t pd_idx = pd_index(virt);
    uint64_t pd_entry = pd->entries[pd_idx];
    if (!(pd_entry & VMM_PRESENT)) return 0;
    
    page_table_t *pt = (page_table_t*)(uintptr_t)(pd_entry & ~0xFFF);
    uint64_t pt_idx = pt_index(virt);
    uint64_t pt_entry = pt->entries[pt_idx];
    if (!(pt_entry & VMM_PRESENT)) return 0;
    
    return (pt_entry & ~0xFFF) | (virt & 0xFFF);
}

/* Get kernel PML4 */
pml4_t *vmm_get_kernel_pml4(void) {
    return kernel_pml4;
}

/* Create a new address space (for new processes) */
pml4_t *vmm_create_address_space(void) {
    void *pml4_page = pmm_alloc_page();
    if (!pml4_page) return NULL;
    
    pml4_t *new_pml4 = (pml4_t*)pml4_page;
    
    /* Clear PML4 */
    for (int i = 0; i < 512; i++) {
        new_pml4->entries[i] = 0;
    }
    
    /* Copy kernel mappings (upper half) */
    if (kernel_pml4) {
        for (int i = 256; i < 512; i++) {
            new_pml4->entries[i] = kernel_pml4->entries[i];
        }
    }
    
    return new_pml4;
}

/* Switch address space */
void vmm_switch_address_space(pml4_t *pml4) {
    if (pml4) {
        vmm_set_cr3((uint64_t)(uintptr_t)pml4);
    }
}

/* Destroy address space (free page tables) */
void vmm_destroy_address_space(pml4_t *pml4) {
    if (!pml4) return;
    
    /* Free user-space page tables (lower half) */
    for (int i = 0; i < 256; i++) {
        if (pml4->entries[i] & VMM_PRESENT) {
            /* TODO: Recursively free page tables */
            /* For now, just clear the entry */
            pml4->entries[i] = 0;
        }
    }
    
    /* Free PML4 itself */
    pmm_free_page(pml4);
}
