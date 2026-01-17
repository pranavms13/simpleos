/*
 * PIT Timer Implementation
 */

#include "timer.h"
#include "../cpu/pic.h"
#include "../io.h"
#include <stddef.h>

/* Timer state */
static uint64_t timer_ticks = 0;
static timer_callback_t timer_callback = NULL;

/* Initialize timer */
void timer_init(uint32_t frequency) {
    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    /* Send command byte */
    outb(PIT_COMMAND, PIT_CHANNEL0_SEL | PIT_ACCESS_LOHI | PIT_MODE2 | PIT_BINARY);
    
    /* Send divisor (low byte, then high byte) */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    
    /* Enable timer interrupt */
    pic_enable_irq(0);
}

/* Timer interrupt handler */
void timer_handler(void) {
    timer_ticks++;
    
    /* Call callback if set */
    if (timer_callback) {
        timer_callback();
    }
}

/* Get current tick count */
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

/* Set timer callback */
void timer_set_callback(timer_callback_t callback) {
    timer_callback = callback;
}
