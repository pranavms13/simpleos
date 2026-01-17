/*
 * PS/2 Keyboard Driver Implementation
 * Scancode set 1 with US QWERTY layout
 */

#include "keyboard.h"
#include "../cpu/pic.h"
#include "../io.h"

/* Keyboard state */
static char key_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0;

/* Modifier key states */
static volatile int shift_pressed = 0;
static volatile int ctrl_pressed = 0;
static volatile int alt_pressed = 0;
static volatile int capslock_on = 0;
static volatile int extended_scancode = 0;

/* Scancode to ASCII table (US QWERTY layout) - 128 entries */
static const char scancode_to_ascii[128] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',  0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0
};

/* Shifted scancode to ASCII table - 128 entries */
static const char scancode_to_ascii_shifted[128] = {
    0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*',  0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   0,   0
};

/* Add character to buffer */
static void buffer_put(char c) {
    int next = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != buffer_tail) {
        key_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

/* Get character from buffer */
static char buffer_get(void) {
    if (buffer_head == buffer_tail) {
        return 0;
    }
    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

/* Initialize keyboard */
void keyboard_init(void) {
    /* Clear buffer */
    buffer_head = 0;
    buffer_tail = 0;
    
    /* Clear modifier states */
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    capslock_on = 0;
    
    /* Enable keyboard interrupt */
    pic_enable_irq(1);
}

/* Keyboard interrupt handler */
void keyboard_handler(void) {
    /* Read scancode from keyboard */
    uint8_t scancode = inb(KEYBOARD_DATA);

    /* Handle extended scancode prefix */
    if (scancode == SCANCODE_EXTENDED) {
        extended_scancode = 1;
        return;
    }
    
    /* Check for key release (bit 7 set) */
    if (scancode & SCANCODE_RELEASE) {
        uint8_t released = scancode & 0x7F;

        if (extended_scancode) {
            extended_scancode = 0;
            return;
        }
        
        /* Handle modifier key release */
        if (released == SCANCODE_LSHIFT || released == SCANCODE_RSHIFT) {
            shift_pressed = 0;
        } else if (released == SCANCODE_LCTRL) {
            ctrl_pressed = 0;
        } else if (released == SCANCODE_LALT) {
            alt_pressed = 0;
        }
        
        return;
    }

    if (extended_scancode) {
        char special = 0;
        switch (scancode) {
            case 0x48:
                special = KEY_UP;
                break;
            case 0x50:
                special = KEY_DOWN;
                break;
            case 0x4B:
                special = KEY_LEFT;
                break;
            case 0x4D:
                special = KEY_RIGHT;
                break;
            default:
                break;
        }
        extended_scancode = 0;
        if (special) {
            buffer_put(special);
        }
        return;
    }
    
    /* Handle modifier key press */
    if (scancode == SCANCODE_LSHIFT || scancode == SCANCODE_RSHIFT) {
        shift_pressed = 1;
        return;
    } else if (scancode == SCANCODE_LCTRL) {
        ctrl_pressed = 1;
        return;
    } else if (scancode == SCANCODE_LALT) {
        alt_pressed = 1;
        return;
    } else if (scancode == SCANCODE_CAPSLOCK) {
        capslock_on = !capslock_on;
        return;
    }
    
    /* Translate scancode to ASCII */
    char c = 0;
    if (scancode < 128) {
        if (shift_pressed) {
            c = scancode_to_ascii_shifted[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }
        
        /* Apply caps lock for letters */
        if (capslock_on && c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        } else if (capslock_on && c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        
        /* Handle Ctrl combinations */
        if (ctrl_pressed && c >= 'a' && c <= 'z') {
            c = c - 'a' + 1;  /* Ctrl+A = 1, etc. */
        }
    }
    
    /* Add to buffer if valid character */
    if (c != 0) {
        buffer_put(c);
    }
}

/* Check if key available */
int keyboard_available(void) {
    return buffer_head != buffer_tail;
}

/* Get character (blocking) */
char keyboard_getchar(void) {
    while (!keyboard_available()) {
        __asm__ volatile("hlt");  /* Wait for interrupt */
    }
    return buffer_get();
}

/* Read characters (non-blocking) */
int keyboard_read(char *buf, size_t n) {
    size_t count = 0;
    
    while (count < n && keyboard_available()) {
        buf[count++] = buffer_get();
    }
    
    return count;
}
