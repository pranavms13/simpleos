/*
 * SimpleOS - UEFI Bootloader with Graphical UI
 * Loads and boots the SimpleOS kernel
 */

#include "efi.h"
#include "graphics.h"
#include "bootinfo.h"
#include "kernel_data.h"  /* Auto-generated: contains kernel_data[] array */

/* Global pointers */
static EFI_SYSTEM_TABLE    *gST;
static EFI_BOOT_SERVICES   *gBS;
static EFI_RUNTIME_SERVICES *gRT;
static EFI_HANDLE          gImageHandle;

/* System info */
static UINT64 total_memory_mb = 0;
static UINT64 usable_memory_mb = 0;
static UINT64 memory_entries = 0;

/* Boot info to pass to kernel */
static boot_info_t boot_info;

/*==============================================================================
 * Helper Functions
 *============================================================================*/

static void wstr_to_str(char *dst, CHAR16 *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = (char)src[i];
        i++;
    }
    dst[i] = 0;
}

static EFI_INPUT_KEY wait_key(void) {
    EFI_INPUT_KEY key;
    EFI_STATUS status;
    
    gST->ConIn->Reset(gST->ConIn, FALSE);
    while ((status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key)) == EFI_NOT_READY)
        ;
    return key;
}

static void delay_ms(UINTN ms) {
    gBS->Stall(ms * 1000);
}

/*==============================================================================
 * System Information
 *============================================================================*/

static void gather_memory_info(void) {
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR *mmap = NULL;
    UINTN mmap_size = 0, map_key, desc_size;
    UINT32 desc_ver;
    UINT64 total_pages = 0, usable_pages = 0;
    UINTN i;
    
    status = gBS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);
    if (status != EFI_BUFFER_TOO_SMALL) return;
    
    mmap_size += desc_size * 4;
    status = gBS->AllocatePool(EfiLoaderData, mmap_size, (VOID**)&mmap);
    if (EFI_ERROR(status)) return;
    
    status = gBS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);
    if (EFI_ERROR(status)) {
        gBS->FreePool(mmap);
        return;
    }
    
    memory_entries = mmap_size / desc_size;
    
    for (i = 0; i < memory_entries; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)
            ((UINT8*)mmap + i * desc_size);
        total_pages += desc->NumberOfPages;
        
        if (desc->Type == EfiConventionalMemory ||
            desc->Type == EfiBootServicesCode ||
            desc->Type == EfiBootServicesData) {
            usable_pages += desc->NumberOfPages;
        }
    }
    
    total_memory_mb = (total_pages * 4096) / (1024 * 1024);
    usable_memory_mb = (usable_pages * 4096) / (1024 * 1024);
    
    gBS->FreePool(mmap);
}

/*==============================================================================
 * Kernel Loading and Boot
 *============================================================================*/

/* Kernel entry point type */
typedef void (*kernel_entry_t)(boot_info_t *info);

/* Load kernel into memory and prepare boot info */
static EFI_STATUS prepare_kernel(UINT64 *kernel_entry) {
    EFI_STATUS status;
    UINTN kernel_size;
    EFI_PHYSICAL_ADDRESS kernel_addr = 0x100000;  /* Load at 1MB */
    UINTN pages_needed;
    
    /* Calculate kernel size from embedded data */
    kernel_size = KERNEL_SIZE;
    pages_needed = (kernel_size + 4095) / 4096;
    
    /* Allocate memory for kernel */
    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, pages_needed, &kernel_addr);
    if (EFI_ERROR(status)) {
        /* Try allocating anywhere */
        kernel_addr = 0;
        status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages_needed, &kernel_addr);
        if (EFI_ERROR(status)) {
            return status;
        }
    }
    
    /* Copy kernel to allocated memory */
    const UINT8 *src = kernel_data;
    UINT8 *dst = (UINT8*)kernel_addr;
    for (UINTN i = 0; i < kernel_size; i++) {
        dst[i] = src[i];
    }
    
    *kernel_entry = kernel_addr;
    
    /* Set up boot info */
    boot_info.magic = BOOTINFO_MAGIC;
    boot_info.version = 1;
    boot_info.kernel_physical_base = kernel_addr;
    boot_info.kernel_virtual_base = kernel_addr;
    boot_info.kernel_size = kernel_size;
    
    /* Set up framebuffer info */
    if (gfx.gop) {
        boot_info.framebuffer.base = gfx.gop->Mode->FrameBufferBase;
        boot_info.framebuffer.size = gfx.gop->Mode->FrameBufferSize;
        boot_info.framebuffer.width = gfx.gop->Mode->Info->HorizontalResolution;
        boot_info.framebuffer.height = gfx.gop->Mode->Info->VerticalResolution;
        boot_info.framebuffer.pitch = gfx.gop->Mode->Info->PixelsPerScanLine * 4;
        boot_info.framebuffer.bpp = 32;
        
        /* Assume BGRA format (common for UEFI) */
        boot_info.framebuffer.blue_mask_shift = 0;
        boot_info.framebuffer.blue_mask_size = 8;
        boot_info.framebuffer.green_mask_shift = 8;
        boot_info.framebuffer.green_mask_size = 8;
        boot_info.framebuffer.red_mask_shift = 16;
        boot_info.framebuffer.red_mask_size = 8;
    }
    
    return EFI_SUCCESS;
}

/* Exit boot services and jump to kernel */
static EFI_STATUS boot_kernel(UINT64 kernel_entry) {
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR *mmap = NULL;
    UINTN mmap_size = 0, map_key, desc_size;
    UINT32 desc_ver;
    
    /* Get memory map size */
    status = gBS->GetMemoryMap(&mmap_size, NULL, &map_key, &desc_size, &desc_ver);
    if (status != EFI_BUFFER_TOO_SMALL) {
        return status;
    }
    
    /* Allocate buffer with extra space (map can grow) */
    mmap_size += desc_size * 16;
    status = gBS->AllocatePool(EfiLoaderData, mmap_size, (VOID**)&mmap);
    if (EFI_ERROR(status)) {
        return status;
    }
    
    /* Get final memory map */
    status = gBS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);
    if (EFI_ERROR(status)) {
        gBS->FreePool(mmap);
        return status;
    }
    
    /* Store memory map in boot info */
    boot_info.mmap = (memory_map_entry_t*)mmap;
    boot_info.mmap_entries = mmap_size / desc_size;
    boot_info.mmap_entry_size = desc_size;
    
    /* Exit boot services - point of no return! */
    status = gBS->ExitBootServices(gImageHandle, map_key);
    if (EFI_ERROR(status)) {
        /* Memory map might have changed, try again */
        mmap_size = 0;
        gBS->GetMemoryMap(&mmap_size, NULL, &map_key, &desc_size, &desc_ver);
        mmap_size += desc_size * 16;
        
        status = gBS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);
        if (EFI_ERROR(status)) {
            return status;
        }
        
        boot_info.mmap_entries = mmap_size / desc_size;
        
        status = gBS->ExitBootServices(gImageHandle, map_key);
        if (EFI_ERROR(status)) {
            return status;
        }
    }
    
    /* We're now outside UEFI - no more boot services! */
    
    /* Jump to kernel */
    kernel_entry_t entry = (kernel_entry_t)kernel_entry;
    entry(&boot_info);
    
    /* Should never return */
    while (1) {
        __asm__ volatile("hlt");
    }
    
    return EFI_SUCCESS;
}

/*==============================================================================
 * UI Drawing
 *============================================================================*/

typedef struct {
    const char *label;
    const char *description;
    int action;
} MENU_ITEM;

static MENU_ITEM menu_items[] = {
    { "Boot SimpleOS",     "Load and execute the SimpleOS kernel",  0 },
    { "UEFI Shell",        "Exit to the UEFI interactive shell",    1 },
    { "Reboot System",     "Restart the computer",                  2 },
    { "Shutdown",          "Power off the computer",                3 },
};

#define MENU_COUNT 4

static void draw_ui(int selected) {
    char buf[64];
    UINT32 panel_x, panel_y, panel_w, panel_h;
    UINT32 info_x, info_y, info_w, info_h;
    UINT32 menu_x, menu_y, btn_w, btn_h;
    UINT32 i;
    
    gfx_clear(COLOR_BG_DARK);
    
    /* Gradient background */
    for (i = 0; i < gfx.height; i += 4) {
        UINT32 color = RGB(20 + (i * 10 / gfx.height), 
                           20 + (i * 15 / gfx.height), 
                           35 + (i * 20 / gfx.height));
        gfx_hline(0, i, gfx.width, color);
        gfx_hline(0, i+1, gfx.width, color);
    }
    
    /* Header */
    gfx_fill_rect(0, 0, gfx.width, 60, RGB(30, 30, 50));
    gfx_text_centered(20, "SimpleOS Bootloader", COLOR_WHITE);
    gfx_text(gfx.width - 100, 22, "v0.3 GUI", COLOR_TEXT_DIM);
    
    /* System info panel */
    panel_w = 500;
    panel_h = 200;
    panel_x = 50;
    panel_y = 100;
    
    gfx_panel(panel_x, panel_y, panel_w, panel_h, "System Information", COLOR_BG_PANEL, RGB(60, 60, 90));
    
    info_x = panel_x + 30;
    info_y = panel_y + 50;
    
    wstr_to_str(buf, gST->FirmwareVendor, 32);
    gfx_text(info_x, info_y, "Firmware:", COLOR_TEXT_DIM);
    gfx_text(info_x + 100, info_y, buf, COLOR_WHITE);
    info_y += 24;
    
    gfx_text(info_x, info_y, "UEFI:", COLOR_TEXT_DIM);
    int_to_str(gST->Hdr.Revision >> 16, buf);
    gfx_text(info_x + 100, info_y, buf, COLOR_WHITE);
    gfx_text(info_x + 100 + gfx_text_width(buf), info_y, ".", COLOR_WHITE);
    int_to_str(gST->Hdr.Revision & 0xFFFF, buf);
    gfx_text(info_x + 108 + gfx_text_width(buf), info_y, buf, COLOR_WHITE);
    info_y += 24;
    
    gfx_text(info_x, info_y, "Display:", COLOR_TEXT_DIM);
    int_to_str(gfx.width, buf);
    gfx_text(info_x + 100, info_y, buf, COLOR_ACCENT2);
    gfx_text(info_x + 100 + gfx_text_width(buf), info_y, " x ", COLOR_TEXT_DIM);
    int_to_str(gfx.height, buf);
    gfx_text(info_x + 130 + gfx_text_width(buf), info_y, buf, COLOR_ACCENT2);
    info_y += 24;
    
    gfx_text(info_x, info_y, "Memory:", COLOR_TEXT_DIM);
    int_to_str(usable_memory_mb, buf);
    gfx_text(info_x + 100, info_y, buf, COLOR_ACCENT);
    gfx_text(info_x + 100 + gfx_text_width(buf), info_y, " MB usable / ", COLOR_TEXT_DIM);
    int_to_str(total_memory_mb, buf);
    gfx_text(info_x + 225, info_y, buf, COLOR_WHITE);
    gfx_text(info_x + 225 + gfx_text_width(buf), info_y, " MB total", COLOR_TEXT_DIM);
    
    /* Memory bar */
    UINT32 bar_x = panel_x + 30;
    UINT32 bar_y = panel_y + panel_h - 35;
    UINT32 bar_w = panel_w - 60;
    gfx_progress(bar_x, bar_y, bar_w, 20, 
                 (UINT32)usable_memory_mb, (UINT32)total_memory_mb,
                 RGB(60, 60, 80), COLOR_ACCENT);
    
    /* Menu panel */
    info_w = 350;
    info_h = 280;
    info_x = gfx.width - info_w - 50;
    info_y = 100;
    
    gfx_panel(info_x, info_y, info_w, info_h, "Boot Menu", COLOR_BG_PANEL, RGB(60, 60, 90));
    
    menu_x = info_x + 25;
    menu_y = info_y + 50;
    btn_w = info_w - 50;
    btn_h = 40;
    
    for (i = 0; i < MENU_COUNT; i++) {
        UINT32 btn_bg = (i == (UINT32)selected) ? COLOR_BG_HOVER : COLOR_BG_BUTTON;
        gfx_button(menu_x, menu_y + i * (btn_h + 10), btn_w, btn_h,
                   menu_items[i].label, btn_bg, COLOR_WHITE, i == (UINT32)selected);
    }
    
    if (selected >= 0 && selected < MENU_COUNT) {
        gfx_text(info_x + 25, info_y + info_h - 35, 
                 menu_items[selected].description, COLOR_TEXT_DIM);
    }
    
    /* Footer */
    UINT32 footer_y = gfx.height - 40;
    gfx_fill_rect(0, footer_y, gfx.width, 40, RGB(25, 25, 40));
    
    gfx_text(30, footer_y + 12, "Use", COLOR_TEXT_DIM);
    gfx_text(60, footer_y + 12, " UP/DOWN ", COLOR_ACCENT);
    gfx_text(140, footer_y + 12, "to navigate,", COLOR_TEXT_DIM);
    gfx_text(260, footer_y + 12, " ENTER ", COLOR_ACCENT);
    gfx_text(320, footer_y + 12, "to select", COLOR_TEXT_DIM);
    
    gfx_text(gfx.width - 200, footer_y + 12, "SimpleOS (c) 2026", COLOR_TEXT_DIM);
}

static void draw_boot_screen(const char *message, UINT32 progress) {
    UINT32 center_x = gfx.width / 2;
    UINT32 center_y = gfx.height / 2;
    
    gfx_clear(COLOR_BG_DARK);
    
    /* Logo */
    gfx_text_centered(center_y - 60, "SimpleOS", COLOR_WHITE);
    gfx_text_centered(center_y - 35, message, COLOR_TEXT_DIM);
    
    /* Progress bar */
    UINT32 bar_w = 300;
    UINT32 bar_x = center_x - bar_w / 2;
    UINT32 bar_y = center_y + 10;
    
    gfx_progress(bar_x, bar_y, bar_w, 24, progress, 100, 
                 COLOR_BG_PANEL, COLOR_ACCENT2);
    
    /* Percentage */
    char pct[8];
    int_to_str(progress, pct);
    int len = 0;
    while (pct[len]) len++;
    pct[len] = '%';
    pct[len + 1] = 0;
    
    UINT32 pct_x = center_x - gfx_text_width(pct) / 2;
    gfx_text(pct_x, bar_y + 35, pct, COLOR_TEXT);
}

/*==============================================================================
 * Entry Point
 *============================================================================*/

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    EFI_INPUT_KEY key;
    int selected = 0;
    int running = 1;
    UINT64 kernel_entry = 0;
    
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gRT = SystemTable->RuntimeServices;
    gImageHandle = ImageHandle;
    
    gBS->SetWatchdogTimer(0, 0, 0, NULL);
    
    status = gfx_init(gBS);
    if (EFI_ERROR(status)) {
        gST->ConOut->ClearScreen(gST->ConOut);
        gST->ConOut->OutputString(gST->ConOut, L"Graphics not available!\r\n");
        wait_key();
        return EFI_SUCCESS;
    }
    
    gather_memory_info();
    
    while (running) {
        draw_ui(selected);
        
        key = wait_key();
        
        switch (key.ScanCode) {
            case 0x01: /* Up */
                if (selected > 0) selected--;
                break;
            case 0x02: /* Down */
                if (selected < MENU_COUNT - 1) selected++;
                break;
            default:
                if (key.UnicodeChar == 0x0D || key.UnicodeChar == ' ') {
                    switch (menu_items[selected].action) {
                        case 0: /* Boot kernel */
                            draw_boot_screen("Preparing kernel...", 0);
                            delay_ms(200);
                            
                            draw_boot_screen("Loading kernel...", 20);
                            status = prepare_kernel(&kernel_entry);
                            
                            if (EFI_ERROR(status)) {
                                gfx_clear(COLOR_BG_DARK);
                                gfx_text_centered(gfx.height/2 - 20, 
                                    "Failed to load kernel!", COLOR_RED);
                                gfx_text_centered(gfx.height/2 + 10, 
                                    "Press any key to return...", COLOR_TEXT_DIM);
                                wait_key();
                                break;
                            }
                            
                            draw_boot_screen("Preparing boot info...", 50);
                            delay_ms(200);
                            
                            draw_boot_screen("Exiting UEFI services...", 80);
                            delay_ms(200);
                            
                            draw_boot_screen("Jumping to kernel...", 100);
                            delay_ms(100);
                            
                            /* Boot the kernel - no return! */
                            boot_kernel(kernel_entry);
                            break;
                            
                        case 1: /* Shell */
                            running = 0;
                            break;
                            
                        case 2: /* Reboot */
                            gfx_clear(COLOR_BG_DARK);
                            gfx_text_centered(gfx.height/2, "Rebooting...", COLOR_ACCENT);
                            delay_ms(500);
                            gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
                            break;
                            
                        case 3: /* Shutdown */
                            gfx_clear(COLOR_BG_DARK);
                            gfx_text_centered(gfx.height/2, "Shutting down...", COLOR_ACCENT);
                            delay_ms(500);
                            gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
                            break;
                    }
                }
                if (key.ScanCode == 0x17) { /* ESC */
                    running = 0;
                }
                break;
        }
    }
    
    gfx_clear(COLOR_BLACK);
    
    return EFI_SUCCESS;
}
