/*
 * Simple graphics library for UEFI GOP framebuffer
 */

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "efi.h"
#include "font.h"

/*==============================================================================
 * Color Definitions (BGRA format for most UEFI systems)
 *============================================================================*/

#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_RED         0x000000FF
#define COLOR_GREEN       0x0000FF00
#define COLOR_BLUE        0x00FF0000
#define COLOR_YELLOW      0x0000FFFF
#define COLOR_CYAN        0x00FFFF00
#define COLOR_MAGENTA     0x00FF00FF
#define COLOR_ORANGE      0x000080FF
#define COLOR_PURPLE      0x00800080
#define COLOR_PINK        0x00CBC0FF
#define COLOR_LIME        0x0000FF80

/* Dark theme colors */
#define COLOR_BG_DARK     0x001E1E2E  /* Dark background */
#define COLOR_BG_PANEL    0x00313244  /* Panel background */
#define COLOR_BG_BUTTON   0x00454545  /* Button background */
#define COLOR_BG_HOVER    0x00585858  /* Button hover */
#define COLOR_ACCENT      0x00F5A97F  /* Accent color (peach) */
#define COLOR_ACCENT2     0x00A6E3A1  /* Secondary accent (green) */
#define COLOR_TEXT        0x00CDD6F4  /* Light text */
#define COLOR_TEXT_DIM    0x00A6ADC8  /* Dimmed text */

/* Make RGB color */
#define RGB(r, g, b) ((UINT32)((b) | ((g) << 8) | ((r) << 16)))

/*==============================================================================
 * Graphics Context
 *============================================================================*/

typedef struct {
    UINT32 *framebuffer;
    UINT32 width;
    UINT32 height;
    UINT32 pitch;  /* Pixels per scanline */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
} GFX_CONTEXT;

static GFX_CONTEXT gfx;

/*==============================================================================
 * Core Graphics Functions
 *============================================================================*/

/* Initialize graphics */
static EFI_STATUS gfx_init(EFI_BOOT_SERVICES *BS) {
    EFI_STATUS status;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    
    status = BS->LocateProtocol(&gop_guid, NULL, (VOID**)&gfx.gop);
    if (EFI_ERROR(status) || gfx.gop == NULL) {
        return status;
    }
    
    gfx.framebuffer = (UINT32*)gfx.gop->Mode->FrameBufferBase;
    gfx.width = gfx.gop->Mode->Info->HorizontalResolution;
    gfx.height = gfx.gop->Mode->Info->VerticalResolution;
    gfx.pitch = gfx.gop->Mode->Info->PixelsPerScanLine;
    
    return EFI_SUCCESS;
}

/* Put a single pixel */
static inline void gfx_pixel(UINT32 x, UINT32 y, UINT32 color) {
    if (x < gfx.width && y < gfx.height) {
        gfx.framebuffer[y * gfx.pitch + x] = color;
    }
}

/* Fill the entire screen */
static void gfx_clear(UINT32 color) {
    UINT32 x, y;
    for (y = 0; y < gfx.height; y++) {
        for (x = 0; x < gfx.width; x++) {
            gfx.framebuffer[y * gfx.pitch + x] = color;
        }
    }
}

/* Draw a filled rectangle */
static void gfx_fill_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color) {
    UINT32 px, py;
    for (py = y; py < y + h && py < gfx.height; py++) {
        for (px = x; px < x + w && px < gfx.width; px++) {
            gfx.framebuffer[py * gfx.pitch + px] = color;
        }
    }
}

/* Draw a rectangle outline */
static void gfx_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color) {
    UINT32 i;
    /* Top and bottom */
    for (i = x; i < x + w && i < gfx.width; i++) {
        if (y < gfx.height) gfx.framebuffer[y * gfx.pitch + i] = color;
        if (y + h - 1 < gfx.height) gfx.framebuffer[(y + h - 1) * gfx.pitch + i] = color;
    }
    /* Left and right */
    for (i = y; i < y + h && i < gfx.height; i++) {
        if (x < gfx.width) gfx.framebuffer[i * gfx.pitch + x] = color;
        if (x + w - 1 < gfx.width) gfx.framebuffer[i * gfx.pitch + x + w - 1] = color;
    }
}

/* Draw a rounded rectangle (filled) */
static void gfx_rounded_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 r, UINT32 color) {
    UINT32 px, py;
    /* Main body */
    gfx_fill_rect(x + r, y, w - 2*r, h, color);
    gfx_fill_rect(x, y + r, r, h - 2*r, color);
    gfx_fill_rect(x + w - r, y + r, r, h - 2*r, color);
    
    /* Corners (simple circle approximation) */
    for (py = 0; py < r; py++) {
        for (px = 0; px < r; px++) {
            UINT32 dx = r - px - 1;
            UINT32 dy = r - py - 1;
            if (dx*dx + dy*dy <= r*r) {
                /* Top-left */
                gfx_pixel(x + px, y + py, color);
                /* Top-right */
                gfx_pixel(x + w - 1 - px, y + py, color);
                /* Bottom-left */
                gfx_pixel(x + px, y + h - 1 - py, color);
                /* Bottom-right */
                gfx_pixel(x + w - 1 - px, y + h - 1 - py, color);
            }
        }
    }
}

/* Draw horizontal line */
static void gfx_hline(UINT32 x, UINT32 y, UINT32 w, UINT32 color) {
    UINT32 i;
    if (y >= gfx.height) return;
    for (i = x; i < x + w && i < gfx.width; i++) {
        gfx.framebuffer[y * gfx.pitch + i] = color;
    }
}

/* Draw vertical line */
static void gfx_vline(UINT32 x, UINT32 y, UINT32 h, UINT32 color) {
    UINT32 i;
    if (x >= gfx.width) return;
    for (i = y; i < y + h && i < gfx.height; i++) {
        gfx.framebuffer[i * gfx.pitch + x] = color;
    }
}

/*==============================================================================
 * Text Rendering
 *============================================================================*/

/* Draw a single character */
static void gfx_char(UINT32 x, UINT32 y, char c, UINT32 fg, UINT32 bg) {
    UINT32 cx, cy;
    const unsigned char *glyph;
    
    if (c < 32 || c > 126) c = '?';
    glyph = font_data[c - 32];
    
    for (cy = 0; cy < FONT_HEIGHT; cy++) {
        unsigned char row = glyph[cy];
        for (cx = 0; cx < FONT_WIDTH; cx++) {
            UINT32 color = (row & (0x80 >> cx)) ? fg : bg;
            if (color != 0) {  /* 0 = transparent */
                gfx_pixel(x + cx, y + cy, color);
            }
        }
    }
}

/* Draw a string */
static void gfx_string(UINT32 x, UINT32 y, const char *str, UINT32 fg, UINT32 bg) {
    while (*str) {
        if (*str == '\n') {
            y += FONT_HEIGHT + 2;
            x = 0;
        } else {
            gfx_char(x, y, *str, fg, bg);
            x += FONT_WIDTH;
        }
        str++;
    }
}

/* Draw a string with transparent background */
static void gfx_text(UINT32 x, UINT32 y, const char *str, UINT32 color) {
    gfx_string(x, y, str, color, 0);
}

/* Get string width in pixels */
static UINT32 gfx_text_width(const char *str) {
    UINT32 w = 0;
    while (*str) {
        if (*str != '\n') w += FONT_WIDTH;
        str++;
    }
    return w;
}

/* Draw centered text */
static void gfx_text_centered(UINT32 y, const char *str, UINT32 color) {
    UINT32 w = gfx_text_width(str);
    UINT32 x = (gfx.width - w) / 2;
    gfx_text(x, y, str, color);
}

/*==============================================================================
 * UI Widgets
 *============================================================================*/

/* Draw a button */
static void gfx_button(UINT32 x, UINT32 y, UINT32 w, UINT32 h, 
                       const char *label, UINT32 bg, UINT32 fg, int selected) {
    UINT32 text_w = gfx_text_width(label);
    UINT32 text_x = x + (w - text_w) / 2;
    UINT32 text_y = y + (h - FONT_HEIGHT) / 2;
    
    /* Draw button background */
    gfx_rounded_rect(x, y, w, h, 8, bg);
    
    /* Draw selection border */
    if (selected) {
        gfx_rect(x, y, w, h, COLOR_ACCENT);
        gfx_rect(x + 1, y + 1, w - 2, h - 2, COLOR_ACCENT);
    }
    
    /* Draw label */
    gfx_text(text_x, text_y, label, fg);
}

/* Draw a panel with title */
static void gfx_panel(UINT32 x, UINT32 y, UINT32 w, UINT32 h, 
                      const char *title, UINT32 bg, UINT32 title_bg) {
    /* Panel body */
    gfx_rounded_rect(x, y, w, h, 12, bg);
    
    /* Title bar */
    gfx_fill_rect(x + 12, y, w - 24, 30, title_bg);
    gfx_fill_rect(x, y + 12, 12, 18, title_bg);
    gfx_fill_rect(x + w - 12, y + 12, 12, 18, title_bg);
    
    /* Title text */
    UINT32 title_x = x + (w - gfx_text_width(title)) / 2;
    gfx_text(title_x, y + 8, title, COLOR_WHITE);
}

/* Draw a progress bar */
static void gfx_progress(UINT32 x, UINT32 y, UINT32 w, UINT32 h,
                         UINT32 value, UINT32 max, UINT32 bg, UINT32 fg) {
    UINT32 fill_w = (value * (w - 4)) / max;
    
    gfx_rounded_rect(x, y, w, h, h/2, bg);
    if (fill_w > 0) {
        gfx_rounded_rect(x + 2, y + 2, fill_w, h - 4, (h-4)/2, fg);
    }
}

/* Integer to string helper */
static void int_to_str(UINT64 num, char *buf) {
    char tmp[32];
    int i = 0, j = 0;
    
    if (num == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    
    while (num > 0) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = 0;
}

/* Draw a labeled value */
static void gfx_label_value(UINT32 x, UINT32 y, const char *label, 
                            UINT64 value, const char *unit, UINT32 label_color, UINT32 value_color) {
    char buf[32];
    UINT32 label_w;
    
    gfx_text(x, y, label, label_color);
    label_w = gfx_text_width(label);
    
    int_to_str(value, buf);
    gfx_text(x + label_w, y, buf, value_color);
    
    if (unit) {
        gfx_text(x + label_w + gfx_text_width(buf), y, unit, label_color);
    }
}

#endif /* _GRAPHICS_H_ */
