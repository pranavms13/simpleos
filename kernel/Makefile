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

# Sources
C_SRCS := kernel.c
ASM_SRCS := entry.S

C_OBJS := $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o))
ASM_OBJS := $(addprefix $(BUILD_DIR)/,$(ASM_SRCS:.S=.o))
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

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Header dependencies
HEADERS := io.h pci.h virtio.h net.h virtio_net.h $(INCLUDE_DIR)/bootinfo.h

# Compile C files
$(BUILD_DIR)/%.o: %.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S files
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
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
