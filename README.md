# SimpleOS - A Minimal Operating System

A complete bootloader + kernel built from scratch. Boots from UEFI, displays a graphical menu, and loads a custom kernel with full OS features including CPU scheduling, memory management, device drivers, system calls, and interrupt handling.

## Features

### Bootloader
- Graphical UI with dark theme
- System information display (firmware, memory, display)
- Interactive boot menu (arrow keys + Enter)
- Embedded kernel loading
- Reboot/shutdown support

### Kernel (v0.2)

#### CPU Scheduling
- Priority-based preemptive scheduler
- 32 priority levels (0 = highest)
- Per-priority ready queues
- Time-slice based preemption
- Process sleep/wake support

#### Memory Management
- **Physical Memory Manager (PMM)**: Bitmap allocator for 4KB pages
- **Virtual Memory Manager (VMM)**: 4-level paging (PML4)
- **Kernel Heap**: First-fit allocator with coalescing (kmalloc/kfree)

#### Device Drivers
- **PIT Timer**: 100Hz timer for scheduling
- **PS/2 Keyboard**: Scancode set 1, US QWERTY, modifier keys

#### System Calls (SYSCALL/SYSRET)
- Modern fast syscall mechanism
- 6 initial syscalls: read, write, exit, yield, sleep, getpid

#### Interrupt Handling
- **GDT**: Kernel/user code and data segments + TSS
- **IDT**: 256 interrupt vectors (exceptions + 16 IRQs)
- **PIC**: 8259 driver with IRQ remapping to INT 32-47
- Assembly ISR stubs with full register save/restore

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
├── boot/                       # UEFI bootloader source
│   ├── main.c                 # Bootloader + UI
│   ├── efi.h                  # UEFI types & protocols
│   ├── graphics.h             # Drawing primitives
│   ├── font.h                 # 8x16 bitmap font
│   └── Makefile
├── kernel/                     # Kernel source
│   ├── entry.S                # Entry point (assembly)
│   ├── kernel.c               # Kernel main + console
│   ├── kernel.ld              # Linker script
│   ├── io.h                   # Port I/O functions
│   ├── cpu/                   # CPU management
│   │   ├── gdt.c / gdt.h     # Global Descriptor Table + TSS
│   │   ├── idt.c / idt.h     # Interrupt Descriptor Table
│   │   ├── isr.S             # Interrupt service routines
│   │   └── pic.c / pic.h     # 8259 PIC driver
│   ├── mm/                    # Memory management
│   │   ├── pmm.c / pmm.h     # Physical memory manager
│   │   ├── vmm.c / vmm.h     # Virtual memory manager
│   │   └── heap.c / heap.h   # Kernel heap allocator
│   ├── syscall/               # System calls
│   │   ├── syscall.c / .h    # SYSCALL/SYSRET handler
│   │   └── syscall_entry.S   # Assembly entry point
│   ├── drivers/               # Device drivers
│   │   ├── timer.c / timer.h # PIT timer driver
│   │   └── keyboard.c / .h   # PS/2 keyboard driver
│   ├── proc/                  # Process management
│   │   ├── process.c / .h    # Process structures (PCB)
│   │   ├── scheduler.c / .h  # Priority scheduler
│   │   └── context.S         # Context switch assembly
│   └── Makefile
├── include/                    # Shared headers
│   └── bootinfo.h             # Boot info structure
├── scripts/
│   └── bin2h.py               # Binary to C header converter
├── build/                      # Build output (generated)
├── Makefile                   # Top-level build
├── run.sh                     # Build & run script
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
8. **Kernel** initializes all subsystems in order:
   - GDT/TSS, PIC, IDT (interrupt handling)
   - PMM, VMM, Heap (memory management)
   - SYSCALL/SYSRET (system calls)
   - Timer, Keyboard (device drivers)
   - Scheduler (CPU scheduling)
9. **Kernel** creates demo threads and enters main loop

## Kernel Subsystems

### Interrupt Handling

The kernel sets up proper interrupt handling with:

```
GDT Layout:
  Entry 0: Null descriptor
  Entry 1: Kernel code (DPL=0)
  Entry 2: Kernel data (DPL=0)
  Entry 3: User code (DPL=3)
  Entry 4: User data (DPL=3)
  Entry 5-6: TSS (for ring transitions)

IDT Layout:
  INT 0-31:  CPU exceptions
  INT 32-47: Hardware IRQs (via 8259 PIC)
  INT 128:   Legacy syscall (optional)
```

### Memory Management

```
Physical Memory:
  - Bitmap allocator (1 bit per 4KB page)
  - Parses UEFI memory map
  - Tracks free/used pages

Virtual Memory:
  - 4-level paging (PML4 → PDPT → PD → PT)
  - Supports per-process address spaces
  - User/Kernel space separation

Kernel Heap:
  - First-fit allocator
  - Free block coalescing
  - kmalloc/kfree API
```

### Process Scheduling

```
Scheduler Design:
  - 32 priority levels (0 = highest)
  - Ready queue per priority level
  - Preemptive (timer-based)
  - Time slices based on priority

Process Control Block:
  - PID, state, priority
  - CPU context (all registers)
  - Kernel stack pointer
  - Page table pointer
```

### System Calls

| Number | Name | Description |
|--------|------|-------------|
| 0 | sys_read | Read from file descriptor |
| 1 | sys_write | Write to file descriptor |
| 2 | sys_exit | Terminate process |
| 3 | sys_yield | Yield CPU to scheduler |
| 4 | sys_sleep | Sleep for milliseconds |
| 5 | sys_getpid | Get process ID |

## Memory Layout

```
0x0000000000000000 - 0x00000000000FFFFF   Reserved / Legacy
0x0000000000100000 - 0x00000000001FFFFF   Kernel (loaded here)
0x0000000000400000 - 0x00000000004FFFFF   Kernel Heap (1MB)
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

- `kernel/entry.S` - First code that runs in kernel
- `kernel/kernel.c` - Main kernel with initialization
- `kernel/cpu/gdt.c` - GDT and TSS setup
- `kernel/cpu/idt.c` - IDT setup and interrupt dispatch
- `kernel/mm/pmm.c` - Physical page allocator
- `kernel/mm/vmm.c` - Virtual memory / paging
- `kernel/proc/scheduler.c` - Priority scheduler
- `boot/main.c` - Bootloader with GUI

## Future Extensions

The kernel now provides a solid foundation for:

- User-mode processes with separate address spaces
- ELF binary loading
- Filesystem (FAT32 or ext2)
- Virtual file system layer
- Networking (TCP/IP stack)
- Shell and user programs

## License

MIT License - Use this as a starting point for your own OS!

## Resources

- [OSDev Wiki](https://wiki.osdev.org/) - Essential OS development resource
- [UEFI Specification](https://uefi.org/specifications)
- [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - CPU reference
