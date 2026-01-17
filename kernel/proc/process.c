/*
 * Process Management Implementation
 */

#include "process.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../cpu/gdt.h"

/* Process table */
static process_t processes[MAX_PROCESSES];
static process_t *current_process = NULL;
static uint64_t next_pid = 1;

/* Idle process (runs when no other process is ready) */
static process_t *idle_process __attribute__((unused)) = NULL;

/* Initialize process subsystem */
void process_init(void) {
    /* Clear process table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].state = PROC_UNUSED;
        processes[i].pid = 0;
    }
    
    current_process = NULL;
    next_pid = 1;
}

/* Find a free slot in process table */
static process_t *alloc_process_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) {
            return &processes[i];
        }
    }
    return NULL;
}

/* Create a new kernel thread */
process_t *process_create_kernel_thread(const char *name, void (*entry)(void), int priority) {
    process_t *proc = alloc_process_slot();
    if (!proc) return NULL;
    
    /* Allocate kernel stack */
    void *stack = pmm_alloc_pages(KERNEL_STACK_SIZE / PAGE_SIZE);
    if (!stack) return NULL;
    
    /* Initialize process */
    proc->pid = next_pid++;
    proc->state = PROC_READY;
    proc->priority = priority;
    proc->page_table = NULL;  /* Kernel thread uses kernel page table */
    proc->parent = current_process;
    proc->exit_code = 0;
    proc->total_time = 0;
    proc->time_slice = 10;  /* Default time slice */
    proc->wake_time = 0;
    proc->next = NULL;
    proc->prev = NULL;
    
    /* Set name */
    int i;
    for (i = 0; i < 31 && name[i]; i++) {
        proc->name[i] = name[i];
    }
    proc->name[i] = '\0';
    
    /* Set up kernel stack */
    proc->kernel_stack = (uint64_t)stack;
    proc->kernel_stack_top = proc->kernel_stack + KERNEL_STACK_SIZE;
    
    /* Set up initial context on the stack */
    uint64_t *sp = (uint64_t*)proc->kernel_stack_top;
    
    /* Push initial context (will be "popped" by context_switch) */
    *--sp = (uint64_t)entry;    /* RIP (return address) */
    *--sp = 0;                   /* RBX */
    *--sp = 0;                   /* RBP */
    *--sp = 0;                   /* R12 */
    *--sp = 0;                   /* R13 */
    *--sp = 0;                   /* R14 */
    *--sp = 0;                   /* R15 */
    
    proc->context = (context_t*)sp;
    
    return proc;
}

/* Create a new user process (placeholder for now) */
process_t *process_create(const char *name, void (*entry)(void), int priority) {
    /* For now, just create a kernel thread */
    /* TODO: Set up user-mode address space */
    return process_create_kernel_thread(name, entry, priority);
}

/* Exit current process */
void process_exit(int code) {
    if (!current_process) return;
    
    current_process->state = PROC_ZOMBIE;
    current_process->exit_code = code;
    
    /* TODO: Clean up resources, notify parent, reschedule */
    
    /* For now, just halt */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Destroy a process and free resources */
void process_destroy(process_t *proc) {
    if (!proc || proc->state == PROC_UNUSED) return;
    
    /* Free kernel stack */
    if (proc->kernel_stack) {
        pmm_free_pages((void*)proc->kernel_stack, KERNEL_STACK_SIZE / PAGE_SIZE);
    }
    
    /* Free page table if user process */
    if (proc->page_table) {
        vmm_destroy_address_space(proc->page_table);
    }
    
    /* Mark slot as unused */
    proc->state = PROC_UNUSED;
    proc->pid = 0;
}

/* Get current process */
process_t *process_current(void) {
    return current_process;
}

/* Get process by PID */
process_t *process_get(uint64_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROC_UNUSED && processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return NULL;
}

size_t process_get_table(process_t **table) {
    if (table) {
        *table = processes;
    }
    return MAX_PROCESSES;
}

/* Set current process (called by scheduler) */
void process_set_current(process_t *proc) {
    current_process = proc;
    
    /* Update TSS stack pointer for ring transitions */
    if (proc) {
        gdt_set_tss_stack(proc->kernel_stack_top);
    }
}
