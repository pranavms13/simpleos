/*
 * Programmable Interval Timer (PIT) Driver
 * Channel 0, Mode 2 (rate generator)
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>

/* PIT ports */
#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

/* PIT command byte */
#define PIT_CHANNEL0_SEL    0x00
#define PIT_ACCESS_LOHI      0x30    // Access low byte, then high byte
#define PIT_MODE2           0x04    // Rate generator
#define PIT_BINARY           0x00    // Binary mode

/* PIT frequency */
#define PIT_FREQUENCY        1193182  // Base frequency in Hz
#define TIMER_FREQUENCY      100      // Desired frequency (100Hz = 10ms ticks)

/* Function prototypes */
void timer_init(uint32_t frequency);
uint64_t timer_get_ticks(void);
void timer_handler(void);  // Called from IRQ0

/* Callback function type for timer interrupts */
typedef void (*timer_callback_t)(void);
void timer_set_callback(timer_callback_t callback);

#endif /* _TIMER_H_ */
