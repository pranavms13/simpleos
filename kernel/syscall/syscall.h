/*
 * System Call Interface
 * SYSCALL/SYSRET mechanism for x86_64
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>
#include <stddef.h>

/* MSR addresses */
#define MSR_EFER        0xC0000080
#define MSR_STAR        0xC0000081
#define MSR_LSTAR       0xC0000082
#define MSR_CSTAR       0xC0000083
#define MSR_SFMASK      0xC0000084

/* EFER bits */
#define EFER_SCE        (1 << 0)  /* SYSCALL Enable */

/* System call numbers */
#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_EXIT        2
#define SYS_YIELD       3
#define SYS_SLEEP       4
#define SYS_GETPID      5
#define SYS_FORK        6
#define SYS_EXEC        7
#define SYS_WAIT        8
#define SYS_BRK         9

#define SYSCALL_MAX     16

/* Syscall context (registers saved during syscall) */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;    /* RFLAGS (saved by SYSCALL) */
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;    /* Arg 1 */
    uint64_t rsi;    /* Arg 2 */
    uint64_t rdx;    /* Arg 3 */
    uint64_t rcx;    /* RIP (saved by SYSCALL) */
    uint64_t rbx;
    uint64_t rax;    /* Syscall number / return value */
} syscall_context_t;

/* Function prototypes */
void syscall_init(void);
int64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, 
                         uint64_t arg3, uint64_t arg4, uint64_t arg5);

/* Assembly entry point */
extern void syscall_entry(void);

#endif /* _SYSCALL_H_ */
