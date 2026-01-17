/*
 * Priority-Based Scheduler
 * 32 priority levels (0 = highest, 31 = lowest)
 */

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <stdint.h>
#include "process.h"

/* Number of priority levels */
#define SCHED_PRIORITY_LEVELS   32
#define SCHED_PRIORITY_HIGHEST  0
#define SCHED_PRIORITY_LOWEST   31
#define SCHED_PRIORITY_DEFAULT  16
#define SCHED_PRIORITY_IDLE     31

/* Time slice in ticks (higher priority = more time) */
#define SCHED_TIME_SLICE_BASE   5

/* Function prototypes */
void scheduler_init(void);
void schedule(void);                    /* Called from timer interrupt */
void scheduler_add(process_t *proc);    /* Add process to ready queue */
void scheduler_remove(process_t *proc); /* Remove from ready queue */
void scheduler_block(process_t *proc);  /* Block current process */
void scheduler_unblock(process_t *proc);/* Unblock process */
void scheduler_yield(void);             /* Voluntarily yield CPU */
void scheduler_sleep(uint64_t ticks);   /* Sleep for N ticks */
void scheduler_tick(void);              /* Timer tick handler */
void scheduler_start(void);             /* Start scheduler (runs first process) */

#endif /* _SCHEDULER_H_ */
