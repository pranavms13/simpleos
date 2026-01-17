/*
 * Interrupt Descriptor Table (IDT)
 * Handles CPU exceptions, hardware interrupts, and software interrupts
 */

#ifndef _IDT_H_
#define _IDT_H_

#include <stdint.h>

/* IDT entry structure (64-bit) */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;           // Interrupt Stack Table (0-7)
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

/* IDT pointer structure */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

/* Interrupt frame (saved registers on interrupt) */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t interrupt_number;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} interrupt_frame_t;

/* IDT type attributes */
#define IDT_TYPE_INTERRUPT  0x8E    // 64-bit interrupt gate
#define IDT_TYPE_TRAP       0x8F    // 64-bit trap gate
#define IDT_PRESENT         0x80    // Present bit
#define IDT_DPL0            0x00    // Descriptor Privilege Level 0
#define IDT_DPL3            0x60    // Descriptor Privilege Level 3

/* Exception numbers */
#define EXCEPTION_DE        0   // Divide Error
#define EXCEPTION_DB        1   // Debug
#define EXCEPTION_NMI       2   // Non-Maskable Interrupt
#define EXCEPTION_BP        3   // Breakpoint
#define EXCEPTION_OF        4   // Overflow
#define EXCEPTION_BR        5   // Bound Range Exceeded
#define EXCEPTION_UD        6   // Invalid Opcode
#define EXCEPTION_NM        7   // Device Not Available
#define EXCEPTION_DF        8   // Double Fault
#define EXCEPTION_TS        10  // Invalid TSS
#define EXCEPTION_NP        11  // Segment Not Present
#define EXCEPTION_SS        12  // Stack Fault
#define EXCEPTION_GP        13  // General Protection
#define EXCEPTION_PF        14  // Page Fault
#define EXCEPTION_MF        16  // x87 FPU Error
#define EXCEPTION_AC        17  // Alignment Check
#define EXCEPTION_MC        18  // Machine Check
#define EXCEPTION_XM        19  // SIMD Floating Point
#define EXCEPTION_VE        20  // Virtualization

/* IRQ numbers (after remapping) */
#define IRQ_TIMER           0   // Timer (IRQ 0 -> INT 32)
#define IRQ_KEYBOARD         1   // Keyboard (IRQ 1 -> INT 33)

/* Function prototypes */
void idt_init(void);
void idt_set_entry(uint8_t num, uint64_t handler, uint16_t selector, uint8_t type_attr);
void interrupt_handler(interrupt_frame_t *frame);

/* Assembly ISR stubs (defined in isr.S) */
extern void isr0(void);   // Divide Error
extern void isr1(void);   // Debug
extern void isr2(void);   // NMI
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound Range
extern void isr6(void);   // Invalid Opcode
extern void isr7(void);   // Device Not Available
extern void isr8(void);   // Double Fault
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment Not Present
extern void isr12(void);  // Stack Fault
extern void isr13(void);  // General Protection
extern void isr14(void);  // Page Fault
extern void isr16(void);  // x87 FPU Error
extern void isr17(void);  // Alignment Check
extern void isr18(void);  // Machine Check
extern void isr19(void);  // SIMD FP Error
extern void isr20(void);  // Virtualization

/* IRQ handlers */
extern void irq0(void);   // Timer
extern void irq1(void);   // Keyboard
extern void irq2(void);   // Cascade
extern void irq3(void);   // COM2
extern void irq4(void);   // COM1
extern void irq5(void);   // LPT2
extern void irq6(void);   // Floppy
extern void irq7(void);   // LPT1
extern void irq8(void);   // CMOS RTC
extern void irq9(void);   // Free
extern void irq10(void);  // Free
extern void irq11(void);  // Free
extern void irq12(void);  // PS/2 Mouse
extern void irq13(void);  // FPU
extern void irq14(void);  // Primary ATA
extern void irq15(void);  // Secondary ATA

#endif /* _IDT_H_ */
