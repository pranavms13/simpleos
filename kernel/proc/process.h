/*
 * Process Management
 * Process Control Block (PCB) and context structures
 */

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <stdint.h>
#include <stddef.h>
#include "../mm/vmm.h"

/* Maximum processes */
#define MAX_PROCESSES       64
#define KERNEL_STACK_SIZE   8192    /* 8KB per process kernel stack */
#define USER_STACK_SIZE     65536   /* 64KB user stack */

/* Process states */
typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE
} proc_state_t;

/* CPU context saved during context switch */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rip;    /* Return address (pushed by call) */
} context_t;

/* Process Control Block */
typedef struct process {
    uint64_t pid;               /* Process ID */
    proc_state_t state;         /* Current state */
    int priority;               /* Priority (0 = highest, 31 = lowest) */
    
    /* CPU context */
    context_t *context;         /* Saved context pointer (stack pointer) */
    uint64_t kernel_stack;      /* Kernel stack base */
    uint64_t kernel_stack_top;  /* Kernel stack top (RSP0 for TSS) */
    
    /* Memory */
    pml4_t *page_table;         /* Page table (NULL for kernel threads) */
    uint64_t brk;               /* Program break (heap end) */
    
    /* Scheduling */
    uint64_t time_slice;        /* Remaining time slice in ticks */
    uint64_t total_time;        /* Total CPU time used */
    uint64_t wake_time;         /* Wake time for sleeping processes */
    
    /* Process tree */
    struct process *parent;     /* Parent process */
    int exit_code;              /* Exit code (when zombie) */
    
    /* Linked list for scheduler queues */
    struct process *next;
    struct process *prev;
    
    /* Name for debugging */
    char name[32];
} process_t;

/* Function prototypes */
void process_init(void);
process_t *process_create(const char *name, void (*entry)(void), int priority);
process_t *process_create_kernel_thread(const char *name, void (*entry)(void), int priority);
void process_exit(int code);
void process_destroy(process_t *proc);
process_t *process_current(void);
process_t *process_get(uint64_t pid);
size_t process_get_table(process_t **table);

/* Context switch (defined in context.S) */
extern void context_switch(context_t **old, context_t *new);

#endif /* _PROCESS_H_ */
