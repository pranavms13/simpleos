/*
 * Interrupt Descriptor Table (IDT) Implementation
 */

#include "idt.h"
#include "pic.h"
#include "../drivers/timer.h"
#include "../drivers/keyboard.h"

/* IDT with 256 entries */
static idt_entry_t idt[256];

/* IDT pointer */
static idt_ptr_t idt_ptr;

/* Exception names for debugging */
static const char *exception_names[] = {
    "Divide Error",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound Range",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD FP Error",
    "Virtualization"
};

/* Set an IDT entry */
void idt_set_entry(uint8_t num, uint64_t handler, uint16_t selector, uint8_t type_attr) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].type_attr = type_attr;
    idt[num].reserved = 0;
}

/* Load IDT using lidt instruction */
static void idt_load(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

/* Initialize IDT */
void idt_init(void) {
    /* Clear IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
    }
    
    /* Set up exception handlers */
    idt_set_entry(EXCEPTION_DE, (uint64_t)isr0, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_DB, (uint64_t)isr1, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_NMI, (uint64_t)isr2, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_BP, (uint64_t)isr3, 0x08, IDT_TYPE_TRAP | IDT_PRESENT | IDT_DPL3);
    idt_set_entry(EXCEPTION_OF, (uint64_t)isr4, 0x08, IDT_TYPE_TRAP | IDT_PRESENT | IDT_DPL3);
    idt_set_entry(EXCEPTION_BR, (uint64_t)isr5, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_UD, (uint64_t)isr6, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_NM, (uint64_t)isr7, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_DF, (uint64_t)isr8, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_TS, (uint64_t)isr10, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_NP, (uint64_t)isr11, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_SS, (uint64_t)isr12, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_GP, (uint64_t)isr13, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_PF, (uint64_t)isr14, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_MF, (uint64_t)isr16, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_AC, (uint64_t)isr17, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_MC, (uint64_t)isr18, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_XM, (uint64_t)isr19, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(EXCEPTION_VE, (uint64_t)isr20, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    
    /* Set up IRQ handlers (remapped to INT 32-47) */
    idt_set_entry(32 + IRQ_TIMER, (uint64_t)irq0, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + IRQ_KEYBOARD, (uint64_t)irq1, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + 2, (uint64_t)irq2, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + 3, (uint64_t)irq3, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + 4, (uint64_t)irq4, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + 5, (uint64_t)irq5, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + 6, (uint64_t)irq6, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(32 + 7, (uint64_t)irq7, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 0, (uint64_t)irq8, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 1, (uint64_t)irq9, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 2, (uint64_t)irq10, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 3, (uint64_t)irq11, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 4, (uint64_t)irq12, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 5, (uint64_t)irq13, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 6, (uint64_t)irq14, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    idt_set_entry(40 + 7, (uint64_t)irq15, 0x08, IDT_TYPE_INTERRUPT | IDT_PRESENT | IDT_DPL0);
    
    /* Load IDT */
    idt_load();
    
    /* NOTE: Interrupts are enabled later by kernel_main after all drivers are ready */
}

/* Default interrupt handler - called from assembly stubs */
void interrupt_handler(interrupt_frame_t *frame) {
    /* Handle exceptions */
    if (frame->interrupt_number < 32) {
        /* CPU exception - for now just halt */
        (void)exception_names;  /* Avoid unused warning */
        __asm__ volatile("cli");
        while (1) {
            __asm__ volatile("hlt");
        }
    }
    /* Handle IRQs */
    else if (frame->interrupt_number >= 32 && frame->interrupt_number < 48) {
        uint8_t irq = frame->interrupt_number - 32;
        
        /* Dispatch to device handlers */
        switch (irq) {
            case IRQ_TIMER:
                timer_handler();
                break;
            case IRQ_KEYBOARD:
                keyboard_handler();
                break;
            default:
                break;
        }
        
        /* Send EOI to PIC */
        pic_eoi(irq);
    }
}
