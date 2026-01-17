/*
 * Global Descriptor Table (GDT) and Task State Segment (TSS)
 * Defines kernel/user segments and TSS for ring transitions
 */

#ifndef _GDT_H_
#define _GDT_H_

#include <stdint.h>

/* GDT entry structure */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

/* GDT pointer structure */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

/* TSS structure for x86_64 (minimal - only what we need) */
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;      // Stack pointer for ring 0
    uint64_t rsp1;      // Stack pointer for ring 1
    uint64_t rsp2;      // Stack pointer for ring 2
    uint64_t reserved1;
    uint64_t ist1;      // Interrupt Stack Table entry 1
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

/* GDT entry indices */
#define GDT_NULL        0
#define GDT_KERNEL_CODE 1
#define GDT_KERNEL_DATA 2
#define GDT_USER_CODE   3
#define GDT_USER_DATA   4
#define GDT_TSS_LOW     5
#define GDT_TSS_HIGH    6

/* Access byte flags */
#define GDT_ACCESS_PRESENT     (1 << 7)
#define GDT_ACCESS_RING0      (0 << 5)
#define GDT_ACCESS_RING3      (3 << 5)
#define GDT_ACCESS_CODE       (1 << 3)
#define GDT_ACCESS_DATA       (0 << 3)
#define GDT_ACCESS_EXEC       (1 << 3)
#define GDT_ACCESS_READ       (1 << 1)
#define GDT_ACCESS_WRITE      (1 << 1)
#define GDT_ACCESS_DIR_DOWN   (1 << 2)

/* Granularity flags */
#define GDT_GRAN_4K       (1 << 3)
#define GDT_GRAN_64BIT    (1 << 1)
#define GDT_GRAN_32BIT    (0 << 1)

/* Function prototypes */
void gdt_init(void);
void gdt_set_tss_stack(uint64_t stack);

#endif /* _GDT_H_ */
