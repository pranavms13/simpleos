/*
 * 8259 Programmable Interrupt Controller (PIC) Driver
 * Remaps IRQs to avoid conflicts with CPU exceptions
 */

#ifndef _PIC_H_
#define _PIC_H_

#include <stdint.h>

/* PIC ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* PIC commands */
#define PIC_EOI         0x20    // End of interrupt
#define PIC_ICW1_INIT   0x11    // Initialization command word 1
#define PIC_ICW1_ICW4   0x01    // ICW4 needed
#define PIC_ICW4_8086   0x01    // 8086 mode

/* IRQ remapping */
#define PIC1_OFFSET     0x20    // Master PIC: IRQ 0-7 -> INT 32-39
#define PIC2_OFFSET     0x28    // Slave PIC: IRQ 8-15 -> INT 40-47

/* Function prototypes */
void pic_init(void);
void pic_eoi(uint8_t irq);
void pic_disable(void);
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);

#endif /* _PIC_H_ */
