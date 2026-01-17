# SimpleOS Kernel Makefile
# Uses x86_64-elf cross-compiler

# Directories
ROOT_DIR := ..
BUILD_DIR := $(ROOT_DIR)/build/kernel
INCLUDE_DIR := $(ROOT_DIR)/include

CC := x86_64-elf-gcc
AS := x86_64-elf-gcc
LD := x86_64-elf-ld
OBJCOPY := x86_64-elf-objcopy

# Output
TARGET := $(BUILD_DIR)/kernel.bin
ELF := $(BUILD_DIR)/kernel.elf

# Sources - Main kernel
C_SRCS := kernel.c

# CPU subsystem
C_SRCS += cpu/gdt.c cpu/pic.c cpu/idt.c
ASM_SRCS := entry.S cpu/isr.S

# Memory management
C_SRCS += mm/pmm.c mm/vmm.c mm/heap.c

# System calls
C_SRCS += syscall/syscall.c
ASM_SRCS += syscall/syscall_entry.S

# Device drivers
C_SRCS += drivers/timer.c drivers/keyboard.c

# Process management
C_SRCS += proc/process.c proc/scheduler.c
ASM_SRCS += proc/context.S

# Shell
C_SRCS += shell/shell.c shell/history.c

# Object files (with proper path transformation)
C_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SRCS))
OBJS := $(ASM_OBJS) $(C_OBJS)

# Linker script
LDSCRIPT := kernel.ld

# Compiler flags (freestanding, no standard library)
CFLAGS := -ffreestanding \
          -fno-stack-protector \
          -fno-pic \
          -mno-red-zone \
          -mcmodel=kernel \
          -Wall \
          -Wextra \
          -std=c11 \
          -O2 \
          -I. \
          -I$(INCLUDE_DIR)

# Assembler flags
ASFLAGS := -ffreestanding

# Linker flags
LDFLAGS := -nostdlib \
           -static \
           -T $(LDSCRIPT)

.PHONY: all clean

all: $(TARGET)

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/cpu
	@mkdir -p $(BUILD_DIR)/mm
	@mkdir -p $(BUILD_DIR)/syscall
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/proc
	@mkdir -p $(BUILD_DIR)/shell

# Compile C files in root directory
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C files in subdirectories
$(BUILD_DIR)/cpu/%.o: cpu/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mm/%.o: mm/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/syscall/%.o: syscall/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: drivers/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/proc/%.o: proc/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell/%.o: shell/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S files in root directory
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Assemble .S files in subdirectories
$(BUILD_DIR)/cpu/%.o: cpu/%.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/syscall/%.o: syscall/%.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/proc/%.o: proc/%.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Link kernel ELF
$(ELF): $(OBJS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# Convert to flat binary
$(TARGET): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $@ ($$(stat -f%z $@ 2>/dev/null || stat -c%s $@) bytes)"

clean:
	rm -rf $(BUILD_DIR)

# Print sources (for debugging)
.PHONY: sources
sources:
	@echo "C Sources: $(C_SRCS)"
	@echo "ASM Sources: $(ASM_SRCS)"
	@echo "Objects: $(OBJS)"
