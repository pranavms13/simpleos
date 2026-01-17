/*
 * Kernel Heap Allocator
 * Simple first-fit allocator with coalescing
 */

#ifndef _HEAP_H_
#define _HEAP_H_

#include <stdint.h>
#include <stddef.h>

/* Function prototypes */
void heap_init(void *start, size_t size);
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t align);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);
size_t kmalloc_size(void *ptr);

/* Statistics */
size_t heap_get_total_size(void);
size_t heap_get_used_size(void);
size_t heap_get_free_size(void);

#endif /* _HEAP_H_ */
