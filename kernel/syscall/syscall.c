/*
 * System Call Implementation
 */

#include "syscall.h"
#include "../cpu/gdt.h"

/* Read MSR */
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/* Write MSR */
static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

/* Syscall handler type - all handlers take 5 args for uniformity */
typedef int64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

/* Forward declarations for syscall handlers */
static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t, uint64_t);
static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t, uint64_t);
static int64_t sys_exit(uint64_t status, uint64_t, uint64_t, uint64_t, uint64_t);
static int64_t sys_yield(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static int64_t sys_sleep(uint64_t ms, uint64_t, uint64_t, uint64_t, uint64_t);
static int64_t sys_getpid(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

/* Syscall table */
static syscall_handler_t syscall_table[SYSCALL_MAX] = {
    [SYS_READ]   = sys_read,
    [SYS_WRITE]  = sys_write,
    [SYS_EXIT]   = sys_exit,
    [SYS_YIELD]  = sys_yield,
    [SYS_SLEEP]  = sys_sleep,
    [SYS_GETPID] = sys_getpid,
};

/* Initialize syscall interface */
void syscall_init(void) {
    /* Enable SYSCALL/SYSRET in EFER */
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= EFER_SCE;
    wrmsr(MSR_EFER, efer);
    
    /* Set up STAR register: segments for SYSCALL/SYSRET */
    /* STAR[47:32] = SYSCALL CS/SS (kernel)
     * STAR[63:48] = SYSRET CS/SS (user)
     * Kernel CS = 0x08, Kernel SS = 0x10
     * User CS = 0x18 | 3 = 0x1B, User SS = 0x20 | 3 = 0x23
     */
    uint64_t star = ((uint64_t)0x18 << 48) | ((uint64_t)0x08 << 32);
    wrmsr(MSR_STAR, star);
    
    /* Set LSTAR to syscall entry point */
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);
    
    /* Set SFMASK to clear IF (interrupts) during syscall */
    wrmsr(MSR_SFMASK, 0x200);  /* Clear IF */
}

/* Syscall dispatcher - called from assembly */
int64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2,
                         uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    if (num >= SYSCALL_MAX || syscall_table[num] == NULL) {
        return -1;  /* Invalid syscall */
    }
    
    return syscall_table[num](arg1, arg2, arg3, arg4, arg5);
}

/*==============================================================================
 * Syscall Implementations
 *============================================================================*/

/* sys_read - read from file descriptor */
static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, 
                        uint64_t unused1, uint64_t unused2) {
    (void)fd;
    (void)buf;
    (void)count;
    (void)unused1;
    (void)unused2;
    /* TODO: Implement when we have a filesystem */
    return -1;
}

/* sys_write - write to file descriptor */
static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count,
                         uint64_t unused1, uint64_t unused2) {
    (void)unused1;
    (void)unused2;
    
    /* For now, only support stdout (fd=1) */
    if (fd != 1) return -1;
    
    /* TODO: Connect to console output */
    const char *str = (const char*)buf;
    (void)str;
    
    return count;
}

/* sys_exit - terminate current process */
static int64_t sys_exit(uint64_t status, uint64_t unused1, uint64_t unused2,
                        uint64_t unused3, uint64_t unused4) {
    (void)status;
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    /* TODO: Implement when we have process management */
    while (1) {
        __asm__ volatile("hlt");
    }
    return 0;
}

/* sys_yield - voluntarily give up CPU */
static int64_t sys_yield(uint64_t unused1, uint64_t unused2, uint64_t unused3,
                         uint64_t unused4, uint64_t unused5) {
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    /* TODO: Call scheduler */
    return 0;
}

/* sys_sleep - sleep for milliseconds */
static int64_t sys_sleep(uint64_t ms, uint64_t unused1, uint64_t unused2,
                         uint64_t unused3, uint64_t unused4) {
    (void)ms;
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    /* TODO: Implement with timer */
    return 0;
}

/* sys_getpid - get current process ID */
static int64_t sys_getpid(uint64_t unused1, uint64_t unused2, uint64_t unused3,
                          uint64_t unused4, uint64_t unused5) {
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    /* TODO: Return actual PID when we have processes */
    return 1;
}
