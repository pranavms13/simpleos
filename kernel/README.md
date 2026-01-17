# SimpleOS Kernel

This directory contains the SimpleOS kernel core: CPU setup, interrupts,
memory management, basic drivers, system call wiring, and process control.

## Core Features

- Interrupt handling with GDT/IDT/PIC setup and ISR stubs
- Physical memory manager (bitmap allocator)
- Virtual memory manager (4-level paging helpers)
- Kernel heap allocator (first-fit with coalescing)
- PIT timer driver and tick counter
- PS/2 keyboard driver with buffered input
- System call entry/dispatch via SYSCALL/SYSRET
- Basic process structures and a priority-based scheduler

## Layout

- `entry.S`: kernel entry point (stack setup, BSS clear, call `kernel_main`)
- `kernel.c`: main initialization sequence and demo loop
- `kernel.ld`: linker script for the kernel image
- `cpu/`: GDT/IDT/PIC setup and ISR stubs
- `mm/`: PMM, VMM, and kernel heap
- `drivers/`: timer and keyboard drivers
- `syscall/`: syscall entry and dispatch
- `proc/`: process and scheduler code

## Usage

Build and run the full system from the repo root:

```
make
make run-gui
```

Key behaviors:

- The kernel prints an initialization banner and phase status messages.
- After init, interrupts are enabled and the keyboard echo loop starts.
- Press keys to see characters echoed on screen.

## Notes

- The kernel currently uses UEFI-provided page tables; custom VMM hooks are present.
- System call support is wired but requires a custom GDT/TSS setup when enabled.
- The scheduler and process system are present but kept minimal for stability.
