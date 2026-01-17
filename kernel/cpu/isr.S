/*
 * Interrupt Service Routine (ISR) Stubs
 * Assembly stubs that save registers and call C handler
 */

.section .text

/* Macro to create ISR stub without error code */
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    push $0              /* Push dummy error code */
    push $\num           /* Push interrupt number */
    jmp isr_common_stub
.endm

/* Macro to create ISR stub with error code */
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    push $\num           /* Push interrupt number (error code already on stack) */
    jmp isr_common_stub
.endm

/* Exception handlers */
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20

/* IRQ handlers */
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47

/* IRQ aliases for convenience */
.global irq0
irq0 = isr32

.global irq1
irq1 = isr33

.global irq2
irq2 = isr34

.global irq3
irq3 = isr35

.global irq4
irq4 = isr36

.global irq5
irq5 = isr37

.global irq6
irq6 = isr38

.global irq7
irq7 = isr39

.global irq8
irq8 = isr40

.global irq9
irq9 = isr41

.global irq10
irq10 = isr42

.global irq11
irq11 = isr43

.global irq12
irq12 = isr44

.global irq13
irq13 = isr45

.global irq14
irq14 = isr46

.global irq15
irq15 = isr47

/* Common ISR stub - saves all registers and calls C handler */
.extern interrupt_handler

isr_common_stub:
    /* Save all general-purpose registers */
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* 
     * Stack alignment check:
     * We pushed 15 registers (120 bytes) + 2 values (16 bytes) = 136 bytes
     * Plus CPU pushed 5 values (40 bytes) = 176 bytes total
     * 176 is divisible by 16, so we're aligned after pushing
     * But we need RSP % 16 == 8 before CALL (since CALL pushes 8 bytes)
     * Current: RSP % 16 == 0, so we need to subtract 8
     */
    sub $8, %rsp
    
    /* Call C handler (frame pointer is in RDI) */
    lea 8(%rsp), %rdi
    call interrupt_handler
    
    /* Restore alignment */
    add $8, %rsp
    
    /* Restore all registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    
    /* Remove interrupt number and error code from stack */
    add $16, %rsp
    
    /* Return from interrupt */
    iretq
