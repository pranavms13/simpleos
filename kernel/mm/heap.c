/*
 * Kernel Heap Allocator Implementation
 * First-fit allocator with free block coalescing
 */

#include "heap.h"
#include "pmm.h"
#include <stddef.h>

/* Heap block header */
typedef struct heap_block {
    size_t size;
    int free;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

/* Heap state */
static heap_block_t *heap_start = NULL;
static size_t heap_total_size = 0;
static size_t heap_used_size = 0;

/* Minimum block size (including header) */
#define MIN_BLOCK_SIZE   (sizeof(heap_block_t) + 16)
#define ALIGN_SIZE       8

/* Align size to ALIGN_SIZE boundary */
static inline size_t align_size(size_t size) {
    return (size + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);
}

/* Initialize heap */
void heap_init(void *start, size_t size) {
    if (!start || size < MIN_BLOCK_SIZE) {
        return;
    }
    
    /* Align start address */
    uintptr_t aligned_start = ((uintptr_t)start + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);
    size -= (aligned_start - (uintptr_t)start);
    size = size & ~(ALIGN_SIZE - 1);
    
    heap_start = (heap_block_t*)aligned_start;
    heap_start->size = size - sizeof(heap_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    
    heap_total_size = size;
    heap_used_size = 0;
}

/* Find first free block that fits */
static heap_block_t *find_free_block(size_t size) {
    heap_block_t *current = heap_start;
    
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/* Split block if it's too large */
static void split_block(heap_block_t *block, size_t size) {
    if (block->size < size + MIN_BLOCK_SIZE) {
        return;  /* Not worth splitting */
    }
    
    /* Create new free block after this one */
    heap_block_t *new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + size);
    new_block->size = block->size - size - sizeof(heap_block_t);
    new_block->free = 1;
    new_block->next = block->next;
    new_block->prev = block;
    
    if (block->next) {
        block->next->prev = new_block;
    }
    
    block->size = size;
    block->next = new_block;
}

/* Coalesce free blocks */
static void coalesce_blocks(heap_block_t *block) {
    /* Coalesce with next block if free */
    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    /* Coalesce with previous block if free */
    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/* Allocate memory */
void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = align_size(size);
    
    heap_block_t *block = find_free_block(size);
    if (!block) {
        return NULL;  /* Out of memory */
    }
    
    /* Split if necessary */
    split_block(block, size);
    
    block->free = 0;
    heap_used_size += block->size + sizeof(heap_block_t);
    
    return (void*)(block + 1);
}

/* Allocate aligned memory */
void *kmalloc_aligned(size_t size, size_t align) {
    if (align < ALIGN_SIZE) align = ALIGN_SIZE;
    if (align & (align - 1)) return NULL;  /* Not power of 2 */
    
    /* Allocate extra space for alignment */
    size_t total_size = size + align + sizeof(heap_block_t);
    void *ptr = kmalloc(total_size);
    if (!ptr) return NULL;
    
    /* Align pointer */
    uintptr_t aligned_ptr = ((uintptr_t)ptr + align - 1) & ~(align - 1);
    
    /* Store original pointer before aligned address */
    *((uintptr_t*)(aligned_ptr - sizeof(uintptr_t))) = (uintptr_t)ptr;
    
    return (void*)aligned_ptr;
}

/* Free memory */
void kfree(void *ptr) {
    if (!ptr) return;
    
    heap_block_t *block = (heap_block_t*)ptr - 1;
    
    if (block->free) {
        return;  /* Double free */
    }
    
    block->free = 1;
    heap_used_size -= block->size + sizeof(heap_block_t);
    
    /* Coalesce with adjacent free blocks */
    coalesce_blocks(block);
}

/* Reallocate memory */
void *krealloc(void *ptr, size_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    heap_block_t *block = (heap_block_t*)ptr - 1;
    size_t old_size = block->size;
    size = align_size(size);
    
    if (size <= old_size) {
        /* Shrink - split if possible */
        split_block(block, size);
        return ptr;
    }
    
    /* Grow - try to coalesce with next block */
    if (block->next && block->next->free) {
        size_t available = old_size + sizeof(heap_block_t) + block->next->size;
        if (available >= size) {
            /* Can expand in place */
            block->next->free = 0;
            block->size = size;
            split_block(block, size);
            return ptr;
        }
    }
    
    /* Need to allocate new block */
    void *new_ptr = kmalloc(size);
    if (!new_ptr) return NULL;
    
    /* Copy data */
    size_t copy_size = old_size < size ? old_size : size;
    for (size_t i = 0; i < copy_size; i++) {
        ((uint8_t*)new_ptr)[i] = ((uint8_t*)ptr)[i];
    }
    
    kfree(ptr);
    return new_ptr;
}

/* Get size of allocated block */
size_t kmalloc_size(void *ptr) {
    if (!ptr) return 0;
    
    heap_block_t *block = (heap_block_t*)ptr - 1;
    return block->size;
}

/* Get statistics */
size_t heap_get_total_size(void) {
    return heap_total_size;
}

size_t heap_get_used_size(void) {
    return heap_used_size;
}

size_t heap_get_free_size(void) {
    return heap_total_size - heap_used_size;
}
