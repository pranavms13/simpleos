/*
 * PS/2 Keyboard Driver
 * Scancode set 1 translation
 */

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <stdint.h>
#include <stddef.h>

/* Keyboard ports */
#define KEYBOARD_DATA    0x60
#define KEYBOARD_STATUS  0x64
#define KEYBOARD_COMMAND 0x64

/* Keyboard status bits */
#define KEYBOARD_STATUS_OUTPUT_FULL  0x01
#define KEYBOARD_STATUS_INPUT_FULL   0x02

/* Scancode set 1 special keys */
#define SCANCODE_ESC         0x01
#define SCANCODE_BACKSPACE   0x0E
#define SCANCODE_TAB         0x0F
#define SCANCODE_ENTER       0x1C
#define SCANCODE_LSHIFT      0x2A
#define SCANCODE_RSHIFT      0x36
#define SCANCODE_LCTRL       0x1D
#define SCANCODE_LALT        0x38
#define SCANCODE_CAPSLOCK    0x3A
#define SCANCODE_RELEASE     0x80
#define SCANCODE_EXTENDED    0xE0

/* Key buffer size */
#define KEYBOARD_BUFFER_SIZE 256

/* Special key codes returned by keyboard_getchar() */
#define KEY_UP      0x80
#define KEY_DOWN    0x81
#define KEY_LEFT    0x82
#define KEY_RIGHT   0x83

/* Function prototypes */
void keyboard_init(void);
char keyboard_getchar(void);      // Blocking
int keyboard_read(char *buf, size_t n);  // Non-blocking
void keyboard_handler(void);      // Called from IRQ1
int keyboard_available(void);     // Check if key available

#endif /* _KEYBOARD_H_ */
