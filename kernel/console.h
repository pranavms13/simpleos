#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stdint.h>

/* Console colors (ARGB) */
#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_GREEN       0x0000FF00
#define COLOR_CYAN        0x00FFFF00
#define COLOR_YELLOW      0x0000FFFF
#define COLOR_RED         0x000000FF
#define COLOR_BLUE        0x00FF0000
#define COLOR_DARK_BG     0x00202030

void console_putchar(char c);
void console_print(const char *str);
void console_set_color(uint32_t color);
void console_clear(void);
void print_dec(uint64_t num);
void print_hex(uint64_t num);

#endif /* _CONSOLE_H_ */
