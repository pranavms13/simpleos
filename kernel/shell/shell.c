#include "shell.h"
#include "history.h"
#include "../console.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../proc/process.h"
#include "../io.h"

#define SHELL_MAX_LINE 128
#define SHELL_MAX_ARGS 8

static void shell_print_banner(void);
static void shell_prompt(void);
static void shell_readline(char *buf, size_t size);
static void shell_execute(char *line);

static int str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s && s[n]) n++;
    return n;
}

static void str_copy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    if (!dst || !src || max == 0) return;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int is_space(char c) {
    return c == ' ' || c == '\t';
}

static void print_bytes_human(uint64_t bytes) {
    uint64_t unit = 1024;
    const char *suffix = "B";
    uint64_t whole = bytes;
    uint64_t frac = 0;

    if (bytes >= unit * unit * unit) {
        suffix = "GB";
        whole = bytes / (unit * unit * unit);
        frac = (bytes % (unit * unit * unit)) * 10 / (unit * unit * unit);
    } else if (bytes >= unit * unit) {
        suffix = "MB";
        whole = bytes / (unit * unit);
        frac = (bytes % (unit * unit)) * 10 / (unit * unit);
    } else if (bytes >= unit) {
        suffix = "KB";
        whole = bytes / unit;
        frac = (bytes % unit) * 10 / unit;
    }

    console_print(" (");
    print_dec(whole);
    if (suffix[0] != 'B') {
        console_putchar('.');
        print_dec(frac);
        console_putchar(' ');
    } else {
        console_putchar(' ');
    }
    console_print(suffix);
    console_print(")");
}

static uint64_t parse_uint(const char *s, int *ok) {
    uint64_t value = 0;
    int base = 10;
    size_t i = 0;
    *ok = 0;
    if (!s || !*s) return 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        i = 2;
    }

    for (; s[i]; i++) {
        char c = s[i];
        uint8_t digit;
        if (c >= '0' && c <= '9') {
            digit = (uint8_t)(c - '0');
        } else if (base == 16 && c >= 'a' && c <= 'f') {
            digit = (uint8_t)(10 + c - 'a');
        } else if (base == 16 && c >= 'A' && c <= 'F') {
            digit = (uint8_t)(10 + c - 'A');
        } else {
            return 0;
        }
        value = value * base + digit;
    }

    *ok = 1;
    return value;
}

void shell_init(void) {
    history_init();
    shell_print_banner();
}

void shell_run(void) {
    char line[SHELL_MAX_LINE];
    while (1) {
        shell_prompt();
        shell_readline(line, sizeof(line));
        if (line[0] == '\0') {
            continue;
        }
        history_add(line);
        shell_execute(line);
    }
}

static void shell_print_banner(void) {
    console_set_color(COLOR_CYAN);
    console_print("========================================\n");
    console_set_color(COLOR_GREEN);
    console_print("            SimpleOS Shell\n");
    console_set_color(COLOR_CYAN);
    console_print("========================================\n");
    console_set_color(COLOR_WHITE);
    console_print("Type 'help' to list commands.\n\n");
}

static void shell_prompt(void) {
    console_set_color(COLOR_GREEN);
    console_print("simpleos");
    console_set_color(COLOR_CYAN);
    console_print("> ");
    console_set_color(COLOR_WHITE);
}

static void replace_input(char *buf, size_t size, size_t *len, const char *new_text) {
    while (*len > 0) {
        console_putchar('\b');
        (*len)--;
    }
    buf[0] = '\0';

    if (new_text && *new_text) {
        str_copy(buf, new_text, size);
        *len = str_len(buf);
        console_print(buf);
    }
}

static void shell_readline(char *buf, size_t size) {
    size_t len = 0;
    char saved[SHELL_MAX_LINE];
    int saved_valid = 0;

    buf[0] = '\0';
    history_reset_nav();

    while (1) {
        int c = (unsigned char)keyboard_getchar();
        if (c == KEY_UP || c == KEY_DOWN) {
            if (!saved_valid) {
                str_copy(saved, buf, sizeof(saved));
                saved_valid = 1;
            }
            const char *entry = (c == KEY_UP) ? history_prev() : history_next();
            if (entry) {
                replace_input(buf, size, &len, entry);
            } else {
                replace_input(buf, size, &len, saved_valid ? saved : "");
            }
            continue;
        }

        if (c == '\n') {
            console_putchar('\n');
            buf[len] = '\0';
            return;
        }

        if (c == '\b') {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                console_putchar('\b');
            }
            saved_valid = 0;
            continue;
        }

        if (c >= 32 && c < 127) {
            if (len < size - 1) {
                buf[len++] = (char)c;
                buf[len] = '\0';
                console_putchar((char)c);
            }
            saved_valid = 0;
            continue;
        }
    }
}

static void cmd_help(void) {
    console_set_color(COLOR_CYAN);
    console_print("Available commands:\n");
    console_set_color(COLOR_WHITE);
    console_print("  help        - Show this help\n");
    console_print("  clear       - Clear the screen\n");
    console_print("  echo [text] - Echo text\n");
    console_print("  version     - Show kernel version\n");
    console_print("  uptime      - Show system uptime\n");
    console_print("  meminfo     - Show memory usage\n");
    console_print("  ps          - List processes\n");
    console_print("  hexdump a l - Dump memory at address\n");
    console_print("  reboot      - Reboot the machine\n");
    console_print("  shutdown    - Power off the machine\n");
}

static void cmd_clear(void) {
    console_clear();
}

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        console_print(argv[i]);
        if (i + 1 < argc) {
            console_putchar(' ');
        }
    }
    console_putchar('\n');
}

static void cmd_version(void) {
    console_set_color(COLOR_GREEN);
    console_print("SimpleOS Kernel v0.2\n");
    console_set_color(COLOR_WHITE);
}

static void cmd_uptime(void) {
    uint64_t ticks = timer_get_ticks();
    uint64_t seconds = ticks / TIMER_FREQUENCY;
    console_print("Uptime: ");
    print_dec(seconds);
    console_print("s\n");
}

static const char *state_to_str(proc_state_t state) {
    switch (state) {
        case PROC_READY: return "READY";
        case PROC_RUNNING: return "RUN";
        case PROC_BLOCKED: return "BLOCK";
        case PROC_ZOMBIE: return "ZOMBIE";
        default: return "UNUSED";
    }
}

static void cmd_ps(void) {
    process_t *table = NULL;
    size_t count = process_get_table(&table);

    console_set_color(COLOR_CYAN);
    console_print("PID   STATE   PRIO  NAME\n");
    console_set_color(COLOR_WHITE);

    for (size_t i = 0; i < count; i++) {
        process_t *proc = &table[i];
        if (proc->state == PROC_UNUSED) continue;

        print_dec(proc->pid);
        console_print("   ");
        console_print(state_to_str(proc->state));
        console_print("   ");
        print_dec((uint64_t)proc->priority);
        console_print("    ");
        console_print(proc->name);
        console_putchar('\n');
    }
}

static void cmd_meminfo(void) {
    uint64_t total_pages = pmm_get_total_pages();
    uint64_t free_pages = pmm_get_free_pages();
    uint64_t used_pages = pmm_get_used_pages();
    uint64_t total_bytes = total_pages * 4096;
    uint64_t free_bytes = free_pages * 4096;
    uint64_t used_bytes = used_pages * 4096;

    console_set_color(COLOR_CYAN);
    console_print("Memory (pages):\n");
    console_set_color(COLOR_WHITE);
    console_print("  total: ");
    print_dec(total_pages);
    print_bytes_human(total_bytes);
    console_print("\n  free:  ");
    print_dec(free_pages);
    print_bytes_human(free_bytes);
    console_print("\n  used:  ");
    print_dec(used_pages);
    print_bytes_human(used_bytes);
    console_print("\n");

    console_set_color(COLOR_CYAN);
    console_print("Heap (bytes):\n");
    console_set_color(COLOR_WHITE);
    console_print("  total: ");
    uint64_t heap_total = heap_get_total_size();
    uint64_t heap_free = heap_get_free_size();
    uint64_t heap_used = heap_get_used_size();
    print_dec(heap_total);
    print_bytes_human(heap_total);
    console_print("\n  free:  ");
    print_dec(heap_free);
    print_bytes_human(heap_free);
    console_print("\n  used:  ");
    print_dec(heap_used);
    print_bytes_human(heap_used);
    console_print("\n");
}

static void cmd_hexdump(int argc, char **argv) {
    if (argc < 3) {
        console_print("Usage: hexdump <addr> <len>\n");
        return;
    }

    int ok_addr = 0;
    int ok_len = 0;
    uint64_t addr = parse_uint(argv[1], &ok_addr);
    uint64_t len = parse_uint(argv[2], &ok_len);

    if (!ok_addr || !ok_len || len == 0) {
        console_print("Invalid arguments.\n");
        return;
    }

    const uint8_t *ptr = (const uint8_t *)addr;
    uint64_t offset = 0;
    while (offset < len) {
        print_hex(addr + offset);
        console_print(": ");
        for (int i = 0; i < 16 && offset + i < len; i++) {
            uint8_t byte = ptr[offset + i];
            const char hex[] = "0123456789ABCDEF";
            char out[3];
            out[0] = hex[(byte >> 4) & 0xF];
            out[1] = hex[byte & 0xF];
            out[2] = '\0';
            console_print(out);
            console_putchar(' ');
        }
        console_putchar('\n');
        offset += 16;
    }
}

static void cmd_reboot(void) {
    console_set_color(COLOR_YELLOW);
    console_print("Rebooting...\n");
    console_set_color(COLOR_WHITE);
    outb(0x64, 0xFE);
    while (1) {
        __asm__ volatile("hlt");
    }
}

static void cmd_shutdown(void) {
    console_set_color(COLOR_YELLOW);
    console_print("Shutting down...\n");
    console_set_color(COLOR_WHITE);

    /* QEMU/Bochs/VirtualBox poweroff ports */
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);

    while (1) {
        __asm__ volatile("hlt");
    }
}

static void shell_execute(char *line) {
    char *argv[SHELL_MAX_ARGS];
    int argc = 0;
    char *p = line;

    while (*p && argc < SHELL_MAX_ARGS) {
        while (*p && is_space(*p)) p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && !is_space(*p)) p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }

    if (argc == 0) return;

    if (str_equal(argv[0], "help")) {
        cmd_help();
    } else if (str_equal(argv[0], "clear")) {
        cmd_clear();
    } else if (str_equal(argv[0], "echo")) {
        cmd_echo(argc, argv);
    } else if (str_equal(argv[0], "version")) {
        cmd_version();
    } else if (str_equal(argv[0], "uptime")) {
        cmd_uptime();
    } else if (str_equal(argv[0], "meminfo")) {
        cmd_meminfo();
    } else if (str_equal(argv[0], "ps")) {
        cmd_ps();
    } else if (str_equal(argv[0], "hexdump")) {
        cmd_hexdump(argc, argv);
    } else if (str_equal(argv[0], "reboot")) {
        cmd_reboot();
    } else if (str_equal(argv[0], "shutdown")) {
        cmd_shutdown();
    } else {
        console_set_color(COLOR_RED);
        console_print("Unknown command: ");
        console_set_color(COLOR_WHITE);
        console_print(argv[0]);
        console_putchar('\n');
    }
}
