# SimpleOS - Top-level Makefile
# Builds kernel + bootloader, runs in QEMU

# Directories
BOOT_DIR := boot
KERNEL_DIR := kernel
BUILD_DIR := build
INCLUDE_DIR := include
ESP_DIR := $(BUILD_DIR)/esp/EFI/BOOT

# Output
TARGET := $(ESP_DIR)/BOOTX64.EFI

# QEMU settings
QEMU := qemu-system-x86_64
QEMU_FLAGS := -M q35 -cpu qemu64 -accel tcg -m 256M

# OVMF firmware
OVMF_PATHS := \
    /opt/homebrew/share/qemu/edk2-x86_64-code.fd \
    /usr/local/share/qemu/edk2-x86_64-code.fd \
    /usr/share/OVMF/OVMF_CODE.fd \
    /usr/share/edk2-ovmf/x64/OVMF_CODE.fd

OVMF := $(firstword $(wildcard $(OVMF_PATHS)))

.PHONY: all clean run run-gui run-net debug kernel boot info

all: $(TARGET)

# Build kernel
kernel: FORCE
	@echo "=== Building Kernel ==="
	@$(MAKE) -C $(KERNEL_DIR)

# Build bootloader (includes kernel)
boot: kernel FORCE
	@echo ""
	@echo "=== Building Bootloader ==="
	@$(MAKE) -C $(BOOT_DIR)

FORCE:

# Copy to ESP
$(TARGET): boot
	@mkdir -p $(ESP_DIR)
	@cp $(BUILD_DIR)/boot/BOOTX64.efi $(TARGET)
	@echo 'fs0:\EFI\BOOT\BOOTX64.EFI' > $(BUILD_DIR)/esp/startup.nsh
	@echo ""
	@echo "=== Build Complete ==="
	@ls -la $(TARGET)
	@ls -la $(BUILD_DIR)/kernel/kernel.bin
	@echo ""
	@echo "Run 'make run' to boot in QEMU"

run: $(TARGET)
ifeq ($(OVMF),)
	@echo "ERROR: OVMF firmware not found!"
	@exit 1
endif
	@echo "Starting QEMU..."
	@echo "Press Ctrl+A, X to exit"
	@echo ""
	$(QEMU) $(QEMU_FLAGS) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/esp \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0,hostfwd=tcp::5555-:22 \
		-nographic

run-net: $(TARGET)
ifeq ($(OVMF),)
	@echo "ERROR: OVMF firmware not found!"
	@exit 1
endif
	@echo "Starting QEMU with network (virtio-net)..."
	@echo "Guest IP: 10.0.2.15 (QEMU user-mode)"
	@echo "To ping from host, use: ping 10.0.2.15 (not possible in user mode)"
	@echo "For full networking, use tap/bridge mode."
	@echo ""
	@echo "Press Ctrl+A, X to exit"
	@echo ""
	$(QEMU) $(QEMU_FLAGS) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/esp \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0 \
		-nographic

run-gui: $(TARGET)
ifeq ($(OVMF),)
	@echo "ERROR: OVMF firmware not found!"
	@exit 1
endif
	@echo "Starting QEMU with GUI..."
	$(QEMU) $(QEMU_FLAGS) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/esp \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0

debug: $(TARGET)
	@echo "Starting QEMU in debug mode..."
	$(QEMU) $(QEMU_FLAGS) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/esp \
		-net none \
		-nographic \
		-s -S

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned"

info:
	@echo "SimpleOS Build Information"
	@echo "=========================="
	@echo "Kernel:      $(BUILD_DIR)/kernel/kernel.bin"
	@echo "Bootloader:  $(BUILD_DIR)/boot/BOOTX64.efi"
	@echo "Target:      $(TARGET)"
	@echo "OVMF:        $(OVMF)"
