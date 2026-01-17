/*
 * Kernel Linker Script
 * Produces a flat binary that can be loaded at any address
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

/* Kernel will be loaded at this virtual address by the bootloader */
KERNEL_VADDR = 0x100000;

SECTIONS
{
    . = KERNEL_VADDR;
    
    /* Text section - executable code */
    .text : ALIGN(4096)
    {
        *(.text._start)    /* Entry point first */
        *(.text .text.*)
    }
    
    /* Read-only data */
    .rodata : ALIGN(4096)
    {
        *(.rodata .rodata.*)
    }
    
    /* Initialized data */
    .data : ALIGN(4096)
    {
        *(.data .data.*)
    }
    
    /* Uninitialized data (BSS) */
    .bss : ALIGN(4096)
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* End of kernel */
    . = ALIGN(4096);
    __kernel_end = .;
    
    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu.hash)
        *(.comment)
        *(.note.gnu.build-id)
    }
}
