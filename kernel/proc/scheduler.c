/*
 * Priority-Based Scheduler Implementation
 */

#include "scheduler.h"
#include "process.h"
#include "../drivers/timer.h"
#include "../cpu/gdt.h"

/* Ready queues (one per priority level) */
static process_t *ready_queues[SCHED_PRIORITY_LEVELS];
static uint32_t ready_bitmap = 0;  /* Bitmap of non-empty queues */

/* Sleep queue (sorted by wake time) */
static process_t *sleep_queue = NULL;

/* Scheduler state */
static int scheduler_running = 0;
static uint64_t current_tick = 0;

/* Idle process entry point */
static void idle_entry(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Idle process */
static process_t *idle_proc = NULL;

/* External function from process.c */
extern void process_set_current(process_t *proc);

/* Add process to ready queue */
void scheduler_add(process_t *proc) {
    if (!proc || proc->state == PROC_UNUSED) return;
    
    int prio = proc->priority;
    if (prio < 0) prio = 0;
    if (prio >= SCHED_PRIORITY_LEVELS) prio = SCHED_PRIORITY_LEVELS - 1;
    
    /* Add to end of queue for this priority */
    proc->next = NULL;
    proc->prev = NULL;
    
    if (ready_queues[prio] == NULL) {
        ready_queues[prio] = proc;
        ready_bitmap |= (1 << prio);
    } else {
        /* Find end of queue */
        process_t *tail = ready_queues[prio];
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = proc;
        proc->prev = tail;
    }
    
    proc->state = PROC_READY;
    
    /* Set time slice based on priority */
    proc->time_slice = SCHED_TIME_SLICE_BASE + (SCHED_PRIORITY_LEVELS - prio);
}

/* Remove process from ready queue */
void scheduler_remove(process_t *proc) {
    if (!proc) return;
    
    int prio = proc->priority;
    if (prio < 0) prio = 0;
    if (prio >= SCHED_PRIORITY_LEVELS) prio = SCHED_PRIORITY_LEVELS - 1;
    
    /* Remove from linked list */
    if (proc->prev) {
        proc->prev->next = proc->next;
    } else {
        ready_queues[prio] = proc->next;
    }
    
    if (proc->next) {
        proc->next->prev = proc->prev;
    }
    
    proc->next = NULL;
    proc->prev = NULL;
    
    /* Update bitmap if queue is empty */
    if (ready_queues[prio] == NULL) {
        ready_bitmap &= ~(1 << prio);
    }
}

/* Find highest priority ready process */
static process_t *find_next_process(void) {
    if (ready_bitmap == 0) {
        return idle_proc;
    }
    
    /* Find lowest set bit (highest priority) */
    int prio = __builtin_ctz(ready_bitmap);
    return ready_queues[prio];
}

/* Initialize scheduler */
void scheduler_init(void) {
    /* Clear ready queues */
    for (int i = 0; i < SCHED_PRIORITY_LEVELS; i++) {
        ready_queues[i] = NULL;
    }
    ready_bitmap = 0;
    sleep_queue = NULL;
    scheduler_running = 0;
    current_tick = 0;
    
    /* Initialize process subsystem */
    process_init();
    
    /* Create idle process */
    idle_proc = process_create_kernel_thread("idle", idle_entry, SCHED_PRIORITY_IDLE);
    if (idle_proc) {
        idle_proc->state = PROC_READY;
        /* Don't add to ready queue - it's special */
    }
}

/* Timer tick handler */
void scheduler_tick(void) {
    current_tick++;
    
    /* Wake up sleeping processes */
    while (sleep_queue && sleep_queue->wake_time <= current_tick) {
        process_t *proc = sleep_queue;
        sleep_queue = proc->next;
        if (sleep_queue) {
            sleep_queue->prev = NULL;
        }
        proc->next = NULL;
        proc->prev = NULL;
        
        scheduler_add(proc);
    }
    
    /* Decrement time slice of current process */
    process_t *current = process_current();
    if (current && current->state == PROC_RUNNING) {
        current->total_time++;
        if (current->time_slice > 0) {
            current->time_slice--;
        }
        
        /* Preempt if time slice expired */
        if (current->time_slice == 0) {
            schedule();
        }
    }
}

/* Main scheduling function */
void schedule(void) {
    process_t *current = process_current();
    process_t *next = find_next_process();
    
    if (!next) {
        return;  /* No process to run */
    }
    
    if (current == next) {
        return;  /* Already running this process */
    }
    
    /* Put current process back on ready queue if still runnable */
    if (current && current->state == PROC_RUNNING) {
        current->state = PROC_READY;
        scheduler_add(current);
    }
    
    /* Remove next process from ready queue */
    scheduler_remove(next);
    next->state = PROC_RUNNING;
    process_set_current(next);
    
    /* Switch page tables if needed */
    if (next->page_table) {
        vmm_switch_address_space(next->page_table);
    }
    
    /* Perform context switch */
    if (current) {
        context_switch(&current->context, next->context);
    } else {
        /* First time scheduling - just switch to next */
        context_switch(&current->context, next->context);
    }
}

/* Block current process */
void scheduler_block(process_t *proc) {
    if (!proc) proc = process_current();
    if (!proc) return;
    
    proc->state = PROC_BLOCKED;
    scheduler_remove(proc);
    
    if (proc == process_current()) {
        schedule();
    }
}

/* Unblock process */
void scheduler_unblock(process_t *proc) {
    if (!proc || proc->state != PROC_BLOCKED) return;
    
    scheduler_add(proc);
}

/* Yield CPU voluntarily */
void scheduler_yield(void) {
    process_t *current = process_current();
    if (current) {
        current->time_slice = 0;  /* Force reschedule */
    }
    schedule();
}

/* Sleep for N ticks */
void scheduler_sleep(uint64_t ticks) {
    process_t *current = process_current();
    if (!current) return;
    
    current->wake_time = current_tick + ticks;
    current->state = PROC_BLOCKED;
    scheduler_remove(current);
    
    /* Insert into sleep queue (sorted by wake time) */
    if (!sleep_queue || current->wake_time < sleep_queue->wake_time) {
        current->next = sleep_queue;
        current->prev = NULL;
        if (sleep_queue) {
            sleep_queue->prev = current;
        }
        sleep_queue = current;
    } else {
        process_t *pos = sleep_queue;
        while (pos->next && pos->next->wake_time <= current->wake_time) {
            pos = pos->next;
        }
        current->next = pos->next;
        current->prev = pos;
        if (pos->next) {
            pos->next->prev = current;
        }
        pos->next = current;
    }
    
    schedule();
}

/* Start scheduler (called once to begin multitasking) */
void scheduler_start(void) {
    scheduler_running = 1;
    
    /* Set timer callback */
    timer_set_callback(scheduler_tick);
    
    /* Run first process */
    schedule();
}
