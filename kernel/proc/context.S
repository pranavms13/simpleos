/*
 * Context Switch Assembly
 * Switches between two kernel thread contexts
 *
 * void context_switch(context_t **old, context_t *new)
 *   RDI = pointer to old context pointer (will be updated)
 *   RSI = new context pointer
 */

.section .text
.global context_switch

context_switch:
    /* Save callee-saved registers on current stack */
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* Save current stack pointer to *old */
    mov %rsp, (%rdi)
    
    /* Load new stack pointer */
    mov %rsi, %rsp
    
    /* Restore callee-saved registers from new stack */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    
    /* Return (pops RIP from stack) */
    ret
