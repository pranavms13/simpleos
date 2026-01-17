/*
 * Global Descriptor Table (GDT) Implementation for x86_64
 */

#include "gdt.h"

/* GDT with 7 entries: null, kernel code/data, user code/data, TSS (2 entries) */
static gdt_entry_t gdt[7] __attribute__((aligned(16)));
static tss_t tss __attribute__((aligned(16)));

/* GDT pointer */
static gdt_ptr_t gdt_ptr __attribute__((aligned(16)));

/* Set a GDT entry */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].access = access;
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[index].base_high = (base >> 24) & 0xFF;
}

/* Initialize GDT */
void gdt_init(void) {
    /* Clear GDT */
    for (int i = 0; i < 7; i++) {
        gdt[i].limit_low = 0;
        gdt[i].base_low = 0;
        gdt[i].base_mid = 0;
        gdt[i].access = 0;
        gdt[i].granularity = 0;
        gdt[i].base_high = 0;
    }
    
    /* Entry 0: Null descriptor */
    gdt_set_entry(GDT_NULL, 0, 0, 0, 0);
    
    /* Entry 1: Kernel code segment
     * Access: Present(0x80) + DPL0(0x00) + non-system(0x10) + Executable(0x08) + Readable(0x02)
     * = 0x9A
     * Flags: Long mode(0x20) + Granularity 4K(0x80) = 0xA0
     */
    gdt_set_entry(GDT_KERNEL_CODE, 0, 0xFFFFF, 0x9A, 0xA0);
    
    /* Entry 2: Kernel data segment
     * Access: Present(0x80) + DPL0(0x00) + non-system(0x10) + Writable(0x02) = 0x92
     * Flags: Granularity 4K(0x80) = 0x80 (no Long mode bit for data)
     */
    gdt_set_entry(GDT_KERNEL_DATA, 0, 0xFFFFF, 0x92, 0x80);
    
    /* Entry 3: User code segment
     * Access: Present(0x80) + DPL3(0x60) + non-system(0x10) + Executable(0x08) + Readable(0x02)
     * = 0xFA
     * Flags: Long mode(0x20) + Granularity 4K(0x80) = 0xA0
     */
    gdt_set_entry(GDT_USER_CODE, 0, 0xFFFFF, 0xFA, 0xA0);
    
    /* Entry 4: User data segment
     * Access: Present(0x80) + DPL3(0x60) + non-system(0x10) + Writable(0x02) = 0xF2
     * Flags: Granularity 4K(0x80) = 0x80
     */
    gdt_set_entry(GDT_USER_DATA, 0, 0xFFFFF, 0xF2, 0x80);
    
    /* Clear TSS */
    uint8_t *tss_bytes = (uint8_t*)&tss;
    for (uint32_t i = 0; i < sizeof(tss); i++) {
        tss_bytes[i] = 0;
    }
    tss.iomap_base = sizeof(tss);
    
    /* Entry 5-6: TSS descriptor (16 bytes for 64-bit TSS)
     * TSS descriptor is different - it spans two GDT entries in 64-bit mode
     */
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;
    
    /* TSS Low (Entry 5) */
    gdt[GDT_TSS_LOW].limit_low = tss_limit & 0xFFFF;
    gdt[GDT_TSS_LOW].base_low = tss_base & 0xFFFF;
    gdt[GDT_TSS_LOW].base_mid = (tss_base >> 16) & 0xFF;
    gdt[GDT_TSS_LOW].access = 0x89;  /* Present, 64-bit TSS available */
    gdt[GDT_TSS_LOW].granularity = ((tss_limit >> 16) & 0x0F);
    gdt[GDT_TSS_LOW].base_high = (tss_base >> 24) & 0xFF;
    
    /* TSS High (Entry 6) - contains upper 32 bits of base */
    uint32_t *tss_high = (uint32_t*)&gdt[GDT_TSS_HIGH];
    tss_high[0] = (tss_base >> 32) & 0xFFFFFFFF;
    tss_high[1] = 0;
    
    /* Load GDT */
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    
    __asm__ volatile(
        "lgdt %0\n"
        /* Reload data segments */
        "mov $0x10, %%ax\n"     /* Kernel data selector */
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        "mov $0x00, %%ax\n"     /* Clear FS and GS */
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        /* Far return to reload CS */
        "pushq $0x08\n"         /* Kernel code selector */
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        :
        : "m"(gdt_ptr)
        : "rax", "memory"
    );
    
    /* Load TSS */
    __asm__ volatile("ltr %%ax" : : "a"((uint16_t)(GDT_TSS_LOW * 8)));
}

/* Set TSS stack pointer for ring 0 */
void gdt_set_tss_stack(uint64_t stack) {
    tss.rsp0 = stack;
}
