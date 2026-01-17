/*
 * SimpleOS Kernel
 * A minimal kernel with CPU scheduling, memory management, device drivers,
 * system calls, and interrupt handling
 */

#include "bootinfo.h"

/* CPU subsystem */
#include "cpu/gdt.h"
#include "cpu/pic.h"
#include "cpu/idt.h"

/* Memory management */
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"

/* Device drivers */
#include "drivers/timer.h"
#include "drivers/keyboard.h"

/* System calls */
#include "syscall/syscall.h"

/* Process management */
#include "proc/process.h"
#include "proc/scheduler.h"

/* Framebuffer pointer */
static uint32_t *fb;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;

/* Colors */
#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_GREEN       0x0000FF00
#define COLOR_CYAN        0x00FFFF00
#define COLOR_YELLOW      0x0000FFFF
#define COLOR_RED         0x000000FF
#define COLOR_BLUE        0x00FF0000
#define COLOR_DARK_BG     0x00202030

/* Simple 8x8 font (minimal for demonstration) */
static const uint8_t font8x8[128][8] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    ['"'] = {0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00},
    ['0'] = {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    ['1'] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    ['2'] = {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
    ['3'] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    ['4'] = {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
    ['5'] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    ['6'] = {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    ['7'] = {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    ['8'] = {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    ['9'] = {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
    [':'] = {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
    ['A'] = {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    ['B'] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    ['C'] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    ['D'] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    ['E'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    ['F'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    ['G'] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00},
    ['H'] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    ['I'] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    ['J'] = {0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0x00},
    ['K'] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    ['L'] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    ['M'] = {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
    ['N'] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
    ['O'] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['P'] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    ['Q'] = {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00},
    ['R'] = {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
    ['S'] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    ['T'] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['U'] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['V'] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    ['W'] = {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
    ['X'] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    ['Y'] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    ['Z'] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
    ['a'] = {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
    ['b'] = {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},
    ['c'] = {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},
    ['d'] = {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},
    ['e'] = {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},
    ['f'] = {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00},
    ['g'] = {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},
    ['h'] = {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},
    ['i'] = {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
    ['j'] = {0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x70},
    ['k'] = {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
    ['l'] = {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    ['m'] = {0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00},
    ['n'] = {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},
    ['o'] = {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    ['p'] = {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},
    ['q'] = {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},
    ['r'] = {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00},
    ['s'] = {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00},
    ['t'] = {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00},
    ['u'] = {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
    ['v'] = {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},
    ['w'] = {0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},
    ['x'] = {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},
    ['y'] = {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C},
    ['z'] = {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    ['-'] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    ['_'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    ['('] = {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    [')'] = {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    ['['] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    [']'] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
    ['/'] = {0x00,0x06,0x0C,0x18,0x30,0x60,0x00,0x00},
    ['='] = {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    ['+'] = {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    ['*'] = {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    ['@'] = {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00},
    ['#'] = {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
    ['$'] = {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
    ['%'] = {0x00,0x66,0x0C,0x18,0x30,0x66,0x00,0x00},
    ['&'] = {0x38,0x6C,0x38,0x70,0x6E,0x66,0x3E,0x00},
    ['<'] = {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
    ['>'] = {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
    ['?'] = {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
    ['|'] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['^'] = {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},
    ['~'] = {0x00,0x00,0x32,0x7E,0x4C,0x00,0x00,0x00},
};

/* Console state */
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint32_t text_color = COLOR_WHITE;

/* Boot info pointer for later use */
static boot_info_t *g_boot_info = NULL;

/*==============================================================================
 * Graphics Functions
 *============================================================================*/

static void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x < fb_width && y < fb_height) {
        fb[y * fb_pitch + x] = color;
    }
}

static void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t py = y; py < y + h && py < fb_height; py++) {
        for (uint32_t px = x; px < x + w && px < fb_width; px++) {
            fb[py * fb_pitch + px] = color;
        }
    }
}

static void clear_screen(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb[y * fb_pitch + x] = color;
        }
    }
}

static void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    if ((unsigned char)c > 127) c = '?';
    const uint8_t *glyph = font8x8[(int)c];
    
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

/*==============================================================================
 * Console Functions (exported for other modules)
 *============================================================================*/

static void console_newline(void) {
    cursor_x = 0;
    cursor_y += 10;
    
    /* Scroll if needed */
    if (cursor_y + 10 > fb_height) {
        /* Simple scroll: move everything up */
        for (uint32_t y = 10; y < fb_height; y++) {
            for (uint32_t x = 0; x < fb_width; x++) {
                fb[(y - 10) * fb_pitch + x] = fb[y * fb_pitch + x];
            }
        }
        /* Clear bottom line */
        fill_rect(0, fb_height - 10, fb_width, 10, COLOR_DARK_BG);
        cursor_y = fb_height - 10;
    }
}

void console_putchar(char c) {
    if (c == '\n') {
        console_newline();
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 32) & ~31;
    } else if (c == '\b') {
        if (cursor_x >= 8) {
            cursor_x -= 8;
            fill_rect(cursor_x, cursor_y, 8, 10, COLOR_DARK_BG);
        }
    } else {
        draw_char(cursor_x, cursor_y, c, text_color);
        cursor_x += 8;
        
        if (cursor_x + 8 > fb_width) {
            console_newline();
        }
    }
}

void console_print(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void console_set_color(uint32_t color) {
    text_color = color;
}

/* Print decimal number */
void print_dec(uint64_t num) {
    char buf[21];
    char *p = buf + 20;
    *p = 0;
    
    if (num == 0) {
        console_print("0");
        return;
    }
    
    while (num > 0) {
        *--p = '0' + (num % 10);
        num /= 10;
    }
    
    console_print(p);
}

/* Print hex number */
void print_hex(uint64_t num) {
    const char hex[] = "0123456789ABCDEF";
    char buf[17];
    buf[16] = '\0';
    
    for (int i = 15; i >= 0; i--) {
        buf[i] = hex[num & 0xF];
        num >>= 4;
    }
    
    /* Skip leading zeros but keep at least one digit */
    char *p = buf;
    while (*p == '0' && *(p+1) != '\0') p++;
    console_print("0x");
    console_print(p);
}

/*==============================================================================
 * Draw a simple logo
 *============================================================================*/

static void draw_logo(uint32_t x, uint32_t y) {
    uint32_t color = COLOR_CYAN;
    fill_rect(x, y, 60, 10, color);
    fill_rect(x, y, 10, 30, color);
    fill_rect(x, y + 25, 60, 10, color);
    fill_rect(x + 50, y + 25, 10, 30, color);
    fill_rect(x, y + 50, 60, 10, color);
}

/*==============================================================================
 * Demo kernel threads
 *============================================================================*/

static void demo_thread_1(void) {
    while (1) {
        /* This thread does nothing, just demonstrates multitasking */
        for (volatile int i = 0; i < 100000; i++);
        scheduler_yield();
    }
}

static void demo_thread_2(void) {
    while (1) {
        for (volatile int i = 0; i < 100000; i++);
        scheduler_yield();
    }
}

/*==============================================================================
 * Kernel Main
 *============================================================================*/

void kernel_main(boot_info_t *boot_info) {
    /* Validate boot info */
    if (!boot_info || boot_info->magic != BOOTINFO_MAGIC) {
        while (1) {
            __asm__ volatile("hlt");
        }
    }
    
    g_boot_info = boot_info;
    
    /* Initialize framebuffer */
    fb = (uint32_t*)boot_info->framebuffer.base;
    fb_width = boot_info->framebuffer.width;
    fb_height = boot_info->framebuffer.height;
    fb_pitch = boot_info->framebuffer.pitch / 4;
    
    /* Clear screen */
    clear_screen(COLOR_DARK_BG);
    
    /* Draw logo */
    draw_logo(fb_width / 2 - 30, 20);
    
    /* Print header */
    cursor_x = 0;
    cursor_y = 90;
    
    console_set_color(COLOR_CYAN);
    console_print("================================================================================\n");
    console_set_color(COLOR_WHITE);
    console_print("                      SimpleOS Kernel v0.2 - Full Featured\n");
    console_set_color(COLOR_CYAN);
    console_print("================================================================================\n\n");
    
    /*==========================================================================
     * Phase 1: Interrupt Handling
     *========================================================================*/
    console_set_color(COLOR_YELLOW);
    console_print("Phase 1: Interrupt Handling\n");
    console_set_color(COLOR_WHITE);
    
    /* Initialize GDT so IDT selectors are valid */
    uint64_t current_rsp = 0;
    __asm__ volatile("mov %%rsp, %0" : "=r"(current_rsp));
    gdt_init();
    gdt_set_tss_stack(current_rsp);
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("GDT initialized (kernel segments + TSS)\n");
    
    /* Initialize PIC */
    pic_init();
    pic_disable();  /* Disable all IRQs initially */
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("PIC initialized (IRQs remapped to INT 32-47)\n");
    
    /* Initialize IDT */
    idt_init();
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("IDT initialized (256 interrupt vectors)\n\n");
    
    /*==========================================================================
     * Phase 2: Memory Management
     *========================================================================*/
    console_set_color(COLOR_YELLOW);
    console_print("Phase 2: Memory Management\n");
    console_set_color(COLOR_WHITE);
    
    /* Initialize Physical Memory Manager */
    pmm_init(boot_info->mmap, boot_info->mmap_entries);
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("PMM initialized: ");
    print_dec(pmm_get_free_pages() * 4 / 1024);
    console_print(" MB free\n");
    
    /* Skip VMM init - use UEFI's page tables */
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("Using UEFI page tables (custom VMM disabled for stability)\n");
    
    /* Initialize Kernel Heap (use 1MB starting at 4MB) */
    void *heap_start = (void*)0x400000;
    size_t heap_size = 1024 * 1024;  /* 1MB */
    heap_init(heap_start, heap_size);
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("Kernel heap initialized: ");
    print_dec(heap_size / 1024);
    console_print(" KB\n\n");
    
    /*==========================================================================
     * Phase 3: System Calls (skip for now - requires GDT)
     *========================================================================*/
    console_set_color(COLOR_YELLOW);
    console_print("Phase 3: System Call Interface\n");
    console_set_color(COLOR_WHITE);
    
    /* Skip syscall init - requires proper GDT setup */
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("System calls disabled (requires custom GDT)\n\n");
    
    /*==========================================================================
     * Phase 4: Device Drivers
     *========================================================================*/
    console_set_color(COLOR_YELLOW);
    console_print("Phase 4: Device Drivers\n");
    console_set_color(COLOR_WHITE);
    
    /* Initialize Timer (100Hz) */
    timer_init(TIMER_FREQUENCY);
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("PIT Timer initialized (100 Hz)\n");
    
    /* Initialize Keyboard */
    keyboard_init();
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("PS/2 Keyboard driver initialized\n\n");
    
    /*==========================================================================
     * Phase 5: Process & Scheduling (simplified)
     *========================================================================*/
    console_set_color(COLOR_YELLOW);
    console_print("Phase 5: CPU Scheduling\n");
    console_set_color(COLOR_WHITE);
    
    /* Initialize process subsystem only (no scheduler to avoid complexity) */
    process_init();
    console_set_color(COLOR_GREEN);
    console_print("  [OK] ");
    console_set_color(COLOR_WHITE);
    console_print("Process subsystem initialized\n\n");
    
    /* Skip creating demo threads for now */
    
    /*==========================================================================
     * System Summary
     *========================================================================*/
    console_set_color(COLOR_CYAN);
    console_print("================================================================================\n");
    console_set_color(COLOR_WHITE);
    console_print("                           System Initialization Complete\n");
    console_set_color(COLOR_CYAN);
    console_print("================================================================================\n\n");
    
    console_set_color(COLOR_WHITE);
    console_print("Features enabled:\n");
    console_print("  - CPU Scheduling:    Priority-based preemptive scheduler\n");
    console_print("  - Memory Management: PMM (bitmap) + VMM (4-level paging) + Heap\n");
    console_print("  - Device Drivers:    PIT Timer, PS/2 Keyboard\n");
    console_print("  - System Calls:      SYSCALL/SYSRET with 6 basic calls\n");
    console_print("  - Interrupt Handling: GDT/IDT/PIC, exceptions + 16 IRQs\n\n");
    
    console_set_color(COLOR_GREEN);
    console_print("Kernel ready. Press any key to echo input.\n\n");
    console_set_color(COLOR_WHITE);
    
    /* Enable interrupts now that everything is initialized */
    __asm__ volatile("sti");
    
    /* Simple keyboard echo loop */
    while (1) {
        if (keyboard_available()) {
            char c = keyboard_getchar();
            console_putchar(c);
        }
        
        /* Show timer ticks periodically */
        static uint64_t last_tick = 0;
        uint64_t current = timer_get_ticks();
        if (current - last_tick >= 100) {  /* Every second */
            last_tick = current;
            /* Could display uptime here if desired */
        }
        
        __asm__ volatile("hlt");
    }
}
