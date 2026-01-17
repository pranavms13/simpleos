/*
 * Kernel Entry Point
 * Called from UEFI bootloader after ExitBootServices
 * 
 * Entry conditions:
 *   - Running in Long Mode (64-bit)
 *   - Paging enabled (identity mapped by UEFI)
 *   - RCX = pointer to boot_info_t structure
 *   - Interrupts disabled
 */

.section .text
.global _start
.extern kernel_main

_start:
    /* Clear direction flag */
    cld
    
    /* Save boot_info pointer (RCX in MS ABI) */
    push %rcx
    
    /* Clear BSS section */
    lea __bss_start(%rip), %rdi    /* destination */
    lea __bss_end(%rip), %rcx      /* end */
    sub %rdi, %rcx                 /* length = end - start */
    shr $3, %rcx                   /* divide by 8 (clear 8 bytes at a time) */
    xor %rax, %rax                 /* zero */
    rep stosq                      /* clear BSS */
    
    /* Restore boot_info pointer */
    pop %rcx
    
    /* RCX contains boot_info pointer (MS ABI first argument) */
    /* For System V ABI (which our kernel uses), first arg goes in RDI */
    mov %rcx, %rdi
    
    /* Clear other registers */
    xor %rax, %rax
    xor %rbx, %rbx
    xor %rdx, %rdx
    xor %rsi, %rsi
    xor %r8, %r8
    xor %r9, %r9
    xor %r10, %r10
    xor %r11, %r11
    xor %r12, %r12
    xor %r13, %r13
    xor %r14, %r14
    xor %r15, %r15
    
    /* Call kernel main */
    call kernel_main
    
    /* If kernel_main returns, halt */
halt:
    cli
    hlt
    jmp halt

/* Stack for the kernel (16KB) */
.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:
