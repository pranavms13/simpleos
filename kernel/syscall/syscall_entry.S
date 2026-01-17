/*
 * System Call Entry Point
 * SYSCALL/SYSRET mechanism for x86_64
 *
 * On SYSCALL:
 *   RCX = user RIP (return address)
 *   R11 = user RFLAGS
 *   RAX = syscall number
 *   RDI, RSI, RDX, R10, R8, R9 = arguments 1-6
 */

.section .text
.global syscall_entry
.extern syscall_dispatch

syscall_entry:
    /* Save user stack and switch to kernel stack */
    /* For now, we stay on the same stack since we don't have per-process stacks yet */
    
    /* Save registers that SYSCALL doesn't preserve */
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* Save user RIP and RFLAGS (in RCX and R11) */
    push %rcx           /* User RIP */
    push %r11           /* User RFLAGS */
    
    /* Prepare arguments for syscall_dispatch */
    /* RAX = syscall number (already there)
     * RDI = arg1 (already there)
     * RSI = arg2 (already there)
     * RDX = arg3 (already there)
     * R10 = arg4 (need to move to RCX for C calling convention)
     * R8  = arg5 (already there)
     * R9  = arg6 (already there, but we only use 5 args)
     */
    mov %rax, %rdi      /* syscall number -> arg1 */
    mov %r10, %rcx      /* arg4 -> rcx (C calling convention) */
    
    /* Actually, let's reorganize for our syscall_dispatch signature:
     * syscall_dispatch(num, arg1, arg2, arg3, arg4, arg5)
     * RDI = num (move from RAX)
     * RSI = arg1 (was in RDI, need to save)
     * RDX = arg2 (was in RSI, need to save)
     * RCX = arg3 (was in RDX, need to save)
     * R8  = arg4 (was in R10)
     * R9  = arg5 (was in R8)
     */
    
    /* Save original args */
    push %rdi           /* Save original arg1 */
    push %rsi           /* Save original arg2 */
    push %rdx           /* Save original arg3 */
    push %r10           /* Save original arg4 */
    push %r8            /* Save original arg5 */
    
    /* Set up C calling convention args */
    mov %rax, %rdi      /* num = syscall number */
    mov 32(%rsp), %rsi  /* arg1 = original RDI (at offset 32 after 5 pushes) */
    mov 24(%rsp), %rdx  /* arg2 = original RSI */
    mov 16(%rsp), %rcx  /* arg3 = original RDX */
    mov 8(%rsp), %r8    /* arg4 = original R10 */
    mov 0(%rsp), %r9    /* arg5 = original R8 */
    
    /* Call C dispatcher */
    call syscall_dispatch
    
    /* Clean up saved args */
    add $40, %rsp       /* Pop 5 saved registers */
    
    /* Result is in RAX */
    
    /* Restore user RFLAGS and RIP */
    pop %r11            /* User RFLAGS */
    pop %rcx            /* User RIP */
    
    /* Restore callee-saved registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    
    /* Return to user mode */
    sysretq
