/*
 * 8259 PIC Implementation
 */

#include "pic.h"
#include "../io.h"

/* Send command to PIC */
static inline void pic_send_command(uint8_t pic, uint8_t cmd) {
    outb(pic, cmd);
    io_wait();
}

/* Send data to PIC */
static inline void pic_send_data(uint8_t pic, uint8_t data) {
    outb(pic, data);
    io_wait();
}

/* Initialize PIC - remap IRQs */
void pic_init(void) {
    /* Initialize master PIC */
    pic_send_command(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_send_data(PIC1_DATA, PIC1_OFFSET);        // Vector offset (IRQ 0-7 -> INT 32-39)
    pic_send_data(PIC1_DATA, 1 << 2);             // Slave on IRQ2
    pic_send_data(PIC1_DATA, PIC_ICW4_8086);
    
    /* Initialize slave PIC */
    pic_send_command(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_send_data(PIC2_DATA, PIC2_OFFSET);        // Vector offset (IRQ 8-15 -> INT 40-47)
    pic_send_data(PIC2_DATA, 2);                  // Cascade identity
    pic_send_data(PIC2_DATA, PIC_ICW4_8086);
    
    /* Mask all IRQs initially (except cascade on IRQ2) */
    pic_send_data(PIC1_DATA, 0xFB);  // 1111 1011 - only IRQ2 (cascade) enabled
    pic_send_data(PIC2_DATA, 0xFF);  // All slave IRQs masked
}

/* Send End of Interrupt */
void pic_eoi(uint8_t irq) {
    if (irq >= 8) {
        pic_send_command(PIC2_COMMAND, PIC_EOI);
    }
    pic_send_command(PIC1_COMMAND, PIC_EOI);
}

/* Disable all PIC interrupts */
void pic_disable(void) {
    pic_send_data(PIC1_DATA, 0xFF);
    pic_send_data(PIC2_DATA, 0xFF);
}

/* Enable specific IRQ */
void pic_enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

/* Disable specific IRQ */
void pic_disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}
