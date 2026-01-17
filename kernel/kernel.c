/*
 * SimpleOS Kernel
 * A minimal kernel that receives boot info from UEFI bootloader
 */

#include "bootinfo.h"
#include "virtio_net.h"

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
 * Console Functions
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

static void console_putchar(char c) {
    if (c == '\n') {
        console_newline();
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 32) & ~31;
    } else {
        draw_char(cursor_x, cursor_y, c, text_color);
        cursor_x += 8;
        
        if (cursor_x + 8 > fb_width) {
            console_newline();
        }
    }
}

static void console_print(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

static void console_set_color(uint32_t color) {
    text_color = color;
}

/* Print hex number */
static void print_hex32(uint32_t num) {
    const char hex[] = "0123456789ABCDEF";
    char buf[9];
    buf[8] = '\0';
    
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex[num & 0xF];
        num >>= 4;
    }
    
    /* Skip leading zeros but keep at least one digit */
    char *p = buf;
    while (*p == '0' && *(p+1) != '\0') p++;
    console_print(p);
}

/* Print hex byte (always 2 digits) */
static void print_hex_byte(uint8_t num) {
    char buf[3];
    uint8_t hi = (num >> 4) & 0x0F;
    uint8_t lo = num & 0x0F;
    buf[0] = (hi < 10) ? ('0' + hi) : ('A' + hi - 10);
    buf[1] = (lo < 10) ? ('0' + lo) : ('A' + lo - 10);
    buf[2] = '\0';
    console_print(buf);
}

/* Print decimal number */
static void print_dec(uint64_t num) {
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

/*==============================================================================
 * Kernel Main
 *============================================================================*/

/* Draw a simple logo */
static void draw_logo(uint32_t x, uint32_t y) {
    /* Simple "S" logo made of rectangles */
    uint32_t color = COLOR_CYAN;
    fill_rect(x, y, 60, 10, color);
    fill_rect(x, y, 10, 30, color);
    fill_rect(x, y + 25, 60, 10, color);
    fill_rect(x + 50, y + 25, 10, 30, color);
    fill_rect(x, y + 50, 60, 10, color);
}

void kernel_main(boot_info_t *boot_info) {
    /* Validate boot info */
    if (!boot_info || boot_info->magic != BOOTINFO_MAGIC) {
        /* Can't do much without valid boot info - halt */
        while (1) {
            __asm__ volatile("hlt");
        }
    }
    
    /* Initialize framebuffer */
    fb = (uint32_t*)boot_info->framebuffer.base;
    fb_width = boot_info->framebuffer.width;
    fb_height = boot_info->framebuffer.height;
    fb_pitch = boot_info->framebuffer.pitch / 4;  /* Convert bytes to pixels */
    
    /* Clear screen with dark background */
    clear_screen(COLOR_DARK_BG);
    
    /* Draw logo */
    draw_logo(fb_width / 2 - 30, 30);
    
    /* Print welcome message */
    cursor_x = 0;
    cursor_y = 110;
    
    console_set_color(COLOR_CYAN);
    console_print("================================================================================\n");
    console_set_color(COLOR_WHITE);
    console_print("                          SimpleOS Kernel v0.1\n");
    console_set_color(COLOR_CYAN);
    console_print("================================================================================\n\n");
    
    console_set_color(COLOR_GREEN);
    console_print("[OK] ");
    console_set_color(COLOR_WHITE);
    console_print("Kernel loaded successfully!\n\n");
    
    /* Display boot info */
    console_set_color(COLOR_YELLOW);
    console_print("Boot Information:\n");
    console_set_color(COLOR_WHITE);
    
    console_print("  Framebuffer: ");
    print_dec(fb_width);
    console_print("x");
    print_dec(fb_height);
    console_print("\n");
    
    console_print("  Memory Map:  ");
    print_dec(boot_info->mmap_entries);
    console_print(" entries\n");
    
    /* Count usable memory - limit iterations to prevent hangs */
    uint64_t usable_pages = 0;
    uint64_t total_pages = 0;
    uint64_t max_entries = boot_info->mmap_entries;
    if (max_entries > 128) max_entries = 128;  /* Safety limit */
    
    for (uint64_t i = 0; i < max_entries; i++) {
        memory_map_entry_t *entry = &boot_info->mmap[i];
        /* Sanity check - ignore entries with insane page counts */
        if (entry->num_pages > 0x100000) continue;  /* Max 4GB per entry */
        
        total_pages += entry->num_pages;
        
        if (entry->type == MEMORY_CONVENTIONAL ||
            entry->type == MEMORY_BOOT_CODE ||
            entry->type == MEMORY_BOOT_DATA) {
            usable_pages += entry->num_pages;
        }
    }
    
    /* Calculate MB (pages * 4KB / 1MB = pages / 256) */
    uint64_t total_mb = total_pages / 256;
    uint64_t usable_mb = usable_pages / 256;
    
    console_print("  Total RAM:   ");
    print_dec(total_mb);
    console_print(" MB\n");
    
    console_print("  Usable RAM:  ");
    print_dec(usable_mb);
    console_print(" MB\n\n");
    
    /* Show some system info */
    console_set_color(COLOR_YELLOW);
    console_print("System Status:\n");
    console_set_color(COLOR_WHITE);
    
    console_set_color(COLOR_GREEN);
    console_print("[OK] ");
    console_set_color(COLOR_WHITE);
    console_print("Framebuffer initialized\n");
    
    console_set_color(COLOR_GREEN);
    console_print("[OK] ");
    console_set_color(COLOR_WHITE);
    console_print("Memory map received from bootloader\n");
    
    console_set_color(COLOR_GREEN);
    console_print("[OK] ");
    console_set_color(COLOR_WHITE);
    console_print("UEFI Boot Services exited\n\n");
    
    /* Initialize network driver */
    console_set_color(COLOR_YELLOW);
    console_print("Network Initialization:\n");
    console_set_color(COLOR_WHITE);
    
    console_print("  Scanning PCI bus 0...\n");
    int pci_count = pci_enumerate();
    console_print("  Found ");
    print_dec(pci_count);
    console_print(" PCI device(s)\n");
    
    /* List PCI devices (limit to 4 for display) */
    int display_count = pci_device_count;
    if (display_count > 4) display_count = 4;
    
    for (int i = 0; i < display_count; i++) {
        pci_device_t *pci = &pci_devices[i];
        console_print("    ");
        
        /* Identify known devices */
        if (pci->vendor_id == 0x1AF4) {
            console_set_color(COLOR_GREEN);
            if (pci->device_id == 0x1000) {
                console_print("Virtio Network");
            } else if (pci->device_id == 0x1001) {
                console_print("Virtio Block");
            } else {
                console_print("Virtio Device");
            }
            console_set_color(COLOR_WHITE);
        } else if (pci->vendor_id == 0x8086) {
            console_print("Intel Chipset");
        } else {
            console_print("PCI Device");
        }
        console_print("\n");
    }
    if (pci_device_count > 4) {
        console_print("    + ");
        print_dec(pci_device_count - 4);
        console_print(" more devices\n");
    }
    console_print("\n");
    
    /* Check for virtio-net device */
    pci_device_t *virtio_dev = pci_find_device(0x1AF4, 0x1000);
    
    if (virtio_dev) {
        console_set_color(COLOR_GREEN);
        console_print("[OK] ");
        console_set_color(COLOR_WHITE);
        console_print("Found virtio-net at PCI 0:");
        print_dec(virtio_dev->device);
        console_print("\n");
        
        /* Read BAR0 directly from PCI config space (not from cached struct) */
        uint32_t bar0 = pci_read32(virtio_dev->bus, virtio_dev->device, 
                                    virtio_dev->function, PCI_BAR0);
        
        console_print("  BAR0 value: ");
        print_dec(bar0);  /* Use decimal for now to avoid hex issues */
        console_print("\n");
        
        /* Check if it's an I/O port or memory BAR */
        if (bar0 & 0x01) {
            console_print("  Type: I/O Port\n");
            uint16_t io_base = (uint16_t)(bar0 & 0xFFFC);
            console_print("  I/O Base: ");
            print_dec(io_base);
            console_print("\n");
            
            /* Enable bus master */
            pci_enable_bus_master(virtio_dev);
            console_print("  Bus master enabled\n");
            
            /* Read MAC directly from config space */
            uint8_t mac[6];
            for (int i = 0; i < 6; i++) {
                mac[i] = inb(io_base + 0x14 + i);
            }
            
            console_print("  MAC: ");
            console_set_color(COLOR_CYAN);
            for (int i = 0; i < 6; i++) {
                print_hex_byte(mac[i]);
                if (i < 5) console_print(":");
            }
            console_set_color(COLOR_WHITE);
            console_print("\n\n");
            
            console_set_color(COLOR_GREEN);
            console_print("[OK] ");
            console_set_color(COLOR_WHITE);
            console_print("Virtio-net driver ready!\n");
            console_print("    Network card detected and MAC address read.\n");
            console_print("    Full packet TX/RX requires virtqueue setup.\n");
        } else {
            console_print("  Type: Memory-mapped (MMIO) - not supported\n");
        }
    } else {
        console_set_color(COLOR_YELLOW);
        console_print("[--] ");
        console_set_color(COLOR_WHITE);
        console_print("No virtio-net device found\n");
        console_print("  Run QEMU with: -device virtio-net-pci\n\n");
    }
    
    console_set_color(COLOR_YELLOW);
    console_print("System halted. Press RESET to restart.\n");
    
    /* Halt */
    while (1) {
        __asm__ volatile("hlt");
    }
}
