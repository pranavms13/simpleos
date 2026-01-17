# SimpleOS - A Minimal Operating System

A complete bootloader + kernel built from scratch. Boots from UEFI, displays a graphical menu, and loads a custom kernel with framebuffer console output.

## Features

### Bootloader
- 🎨 **Graphical UI** with dark theme
- 📊 System information display (firmware, memory, display)
- ⌨️ Interactive boot menu (arrow keys + Enter)
- 💾 Embedded kernel loading
- 🔄 Reboot/shutdown support

### Kernel  
- 🖥️ Framebuffer console output
- 📝 8x8 bitmap font rendering
- 🗺️ Memory map from UEFI
- ✅ Runs after ExitBootServices (no UEFI dependencies)

## Quick Start

```bash
# Build everything (kernel + bootloader)
make

# Run in QEMU with GUI
make run-gui

# Or terminal mode
make run
```

**In the bootloader**: Use ↑/↓ to navigate, Enter to select "Boot SimpleOS"

## Requirements

### macOS (Apple Silicon or Intel)
```bash
brew install x86_64-elf-gcc x86_64-elf-binutils  # For kernel
brew install mingw-w64                             # For bootloader
brew install qemu                                  # For testing
```

### Ubuntu/Debian
```bash
sudo apt install build-essential gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu
sudo apt install mingw-w64 qemu-system-x86 ovmf
```

## Project Structure

```
simpleos/
├── boot/                   # UEFI bootloader source
│   ├── main.c             # Bootloader + UI
│   ├── efi.h              # UEFI types & protocols
│   ├── graphics.h         # Drawing primitives
│   ├── font.h             # 8x16 bitmap font
│   └── Makefile
├── kernel/                 # Kernel source
│   ├── entry.S            # Entry point (assembly)
│   ├── kernel.c           # Kernel main + console
│   ├── kernel.ld          # Linker script
│   └── Makefile
├── include/                # Shared headers
│   └── bootinfo.h         # Boot info structure (shared)
├── scripts/
│   └── bin2h.py           # Binary to C header converter
├── build/                  # Build output (generated)
│   ├── boot/              # Bootloader artifacts
│   ├── kernel/            # Kernel artifacts
│   └── esp/EFI/BOOT/      # UEFI boot partition
├── Makefile               # Top-level build
├── run.sh                 # Build & run script
└── README.md
```

## Boot Process

1. **UEFI Firmware** loads `BOOTX64.EFI` from the ESP
2. **Bootloader** initializes graphics, displays menu
3. User selects "Boot SimpleOS"
4. **Bootloader** copies kernel to memory at 0x100000 (1MB)
5. **Bootloader** prepares `boot_info_t` structure with:
   - Framebuffer address and size
   - Memory map from UEFI
6. **Bootloader** calls `ExitBootServices()` - no more UEFI!
7. **Bootloader** jumps to kernel entry point
8. **Kernel** receives `boot_info_t`, initializes framebuffer console
9. **Kernel** displays welcome message and system info

## Boot Info Structure

The bootloader passes this structure to the kernel:

```c
typedef struct {
    uint64_t magic;              // BOOTINFO_MAGIC
    framebuffer_info_t framebuffer;  // Screen info
    memory_map_entry_t *mmap;    // Memory map
    uint64_t mmap_entries;       // Number of entries
    uint64_t kernel_physical_base;
    uint64_t kernel_size;
    // ...
} boot_info_t;
```

## Extending the Kernel

After the kernel receives control, you could add:

```c
// In kernel.c after kernel_main():

// 1. Set up GDT (Global Descriptor Table)
gdt_init();

// 2. Set up IDT (Interrupt Descriptor Table)  
idt_init();

// 3. Initialize physical memory manager
pmm_init(boot_info->mmap, boot_info->mmap_entries);

// 4. Set up paging / virtual memory
vmm_init();

// 5. Initialize kernel heap
heap_init();

// 6. Start device drivers
drivers_init();

// 7. Start scheduler / multitasking
scheduler_init();
```

## Memory Layout

```
0x0000000000000000 - 0x00000000000FFFFF   Reserved / Legacy
0x0000000000100000 - 0x00000000001FFFFF   Kernel (loaded here)
0x0000000080000000 - ...                   Framebuffer (MMIO)
...                                        Free memory (see mmap)
```

## Technical Details

### Why Two Compilers?

| Component | Compiler | ABI | Reason |
|-----------|----------|-----|--------|
| Kernel | x86_64-elf-gcc | System V | Standard for kernels |
| Bootloader | mingw-w64 | Microsoft | Required by UEFI |

### Key Files

- `kernel/entry.S` - First code that runs in kernel, sets up and calls `kernel_main`
- `kernel/kernel.c` - Main kernel with framebuffer console
- `boot/main.c` - Bootloader with GUI and kernel loading
- `boot/efi.h` - Complete UEFI types including GOP
- `include/bootinfo.h` - Boot info structure shared between bootloader and kernel

## License

MIT License - Use this as a starting point for your own OS!

## Resources

- [OSDev Wiki](https://wiki.osdev.org/) - Essential OS development resource
- [UEFI Specification](https://uefi.org/specifications)
- [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - CPU reference
