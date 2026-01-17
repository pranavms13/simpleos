# SimpleOS UEFI Bootloader with embedded kernel
# Uses mingw-w64 for bootloader, x86_64-elf for kernel

# Directories
ROOT_DIR := ..
BUILD_DIR := $(ROOT_DIR)/build/boot
KERNEL_BUILD := $(ROOT_DIR)/build/kernel
INCLUDE_DIR := $(ROOT_DIR)/include

# Bootloader toolchain (mingw-w64 for MS ABI)
CC := x86_64-w64-mingw32-gcc
LD := x86_64-w64-mingw32-gcc

# Output
TARGET := $(BUILD_DIR)/BOOTX64.efi

# Sources
SRCS := main.c
OBJS := $(BUILD_DIR)/main.o

# Kernel binary (built by kernel/Makefile)
KERNEL_BIN := $(KERNEL_BUILD)/kernel.bin
KERNEL_HDR := $(BUILD_DIR)/kernel_data.h

# Compiler flags for UEFI
CFLAGS := -ffreestanding \
          -fno-stack-protector \
          -fno-stack-check \
          -fshort-wchar \
          -mno-red-zone \
          -Wall \
          -Wextra \
          -std=c11 \
          -O2 \
          -I. \
          -I$(INCLUDE_DIR)

# Linker flags for EFI subsystem
LDFLAGS := -nostdlib \
           -Wl,-dll \
           -shared \
           -Wl,--subsystem,10 \
           -e efi_main

.PHONY: all clean

all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Convert kernel binary to C header
$(KERNEL_HDR): $(KERNEL_BIN) | $(BUILD_DIR)
	@echo "Converting kernel to C array..."
	@python3 $(ROOT_DIR)/scripts/bin2h.py $< $@

# Compile bootloader
$(BUILD_DIR)/main.o: main.c efi.h graphics.h font.h $(KERNEL_HDR) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(BUILD_DIR) -c $< -o $@

# Link bootloader
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
	@echo "Built: $@"

clean:
	rm -rf $(BUILD_DIR)
