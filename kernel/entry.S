/*
 * Kernel Entry Point
 * Called from UEFI bootloader after ExitBootServices
 * 
 * Entry conditions:
 *   - Running in Long Mode (64-bit)
 *   - Paging enabled (identity mapped by UEFI)
 *   - RCX = pointer to boot_info_t structure (MS ABI)
 *   - Interrupts disabled
 */

.section .text
.global _start
.extern kernel_main

_start:
    /* Clear direction flag */
    cld
    
    /* Save boot_info pointer (RCX in MS ABI) to a callee-saved register */
    mov %rcx, %r12
    
    /* Set up our own stack FIRST (before touching BSS which contains the stack) */
    lea stack_top(%rip), %rsp
    
    /* Align stack to 16 bytes */
    and $~0xF, %rsp
    
    /* Clear BSS section */
    lea __bss_start(%rip), %rdi    /* destination */
    lea __bss_end(%rip), %rcx      /* end */
    sub %rdi, %rcx                 /* length = end - start */
    test %rcx, %rcx                /* check if length > 0 */
    jz .bss_done
    shr $3, %rcx                   /* divide by 8 (clear 8 bytes at a time) */
    jz .bss_done                   /* if less than 8 bytes, skip */
    xor %rax, %rax                 /* zero */
    rep stosq                      /* clear BSS */
.bss_done:
    
    /* Set up stack again after BSS clear (in case stack_top was in BSS) */
    lea stack_top(%rip), %rsp
    and $~0xF, %rsp
    
    /* Push a fake return address (required for proper stack alignment) */
    push $0
    
    /* Restore boot_info pointer to RDI (System V ABI first argument) */
    mov %r12, %rdi
    
    /* Clear other registers */
    xor %rax, %rax
    xor %rbx, %rbx
    xor %rcx, %rcx
    xor %rdx, %rdx
    xor %rsi, %rsi
    xor %r8, %r8
    xor %r9, %r9
    xor %r10, %r10
    xor %r11, %r11
    /* Keep r12 for now since we used it for boot_info */
    xor %r13, %r13
    xor %r14, %r14
    xor %r15, %r15
    xor %rbp, %rbp
    
    /* Call kernel main */
    call kernel_main
    
    /* If kernel_main returns, halt */
halt:
    cli
    hlt
    jmp halt

/* Stack for the kernel (16KB) - in separate section to not be cleared by BSS init */
.section .data
.align 16
stack_bottom:
    .skip 16384
stack_top:
