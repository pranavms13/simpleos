/*
 * I/O Port and Memory-Mapped I/O Functions
 */

#ifndef _IO_H_
#define _IO_H_

#include <stdint.h>

/*==============================================================================
 * Port I/O (x86 specific)
 *============================================================================*/

/* Read byte from port */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Read word from port */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Read dword from port */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Write byte to port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Write word to port */
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Write dword to port */
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* I/O wait (for slow devices) */
static inline void io_wait(void) {
    outb(0x80, 0);
}

/*==============================================================================
 * Memory-Mapped I/O
 *============================================================================*/

/* Read from memory-mapped register */
static inline uint8_t mmio_read8(void *addr) {
    return *(volatile uint8_t *)addr;
}

static inline uint16_t mmio_read16(void *addr) {
    return *(volatile uint16_t *)addr;
}

static inline uint32_t mmio_read32(void *addr) {
    return *(volatile uint32_t *)addr;
}

static inline uint64_t mmio_read64(void *addr) {
    return *(volatile uint64_t *)addr;
}

/* Write to memory-mapped register */
static inline void mmio_write8(void *addr, uint8_t val) {
    *(volatile uint8_t *)addr = val;
}

static inline void mmio_write16(void *addr, uint16_t val) {
    *(volatile uint16_t *)addr = val;
}

static inline void mmio_write32(void *addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

static inline void mmio_write64(void *addr, uint64_t val) {
    *(volatile uint64_t *)addr = val;
}

/*==============================================================================
 * Memory Barriers
 *============================================================================*/

static inline void memory_barrier(void) {
    __asm__ volatile("mfence" ::: "memory");
}

static inline void read_barrier(void) {
    __asm__ volatile("lfence" ::: "memory");
}

static inline void write_barrier(void) {
    __asm__ volatile("sfence" ::: "memory");
}

#endif /* _IO_H_ */
