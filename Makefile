# Bare-metal cross-toolchain.
TARGET := i686-elf
CC := $(TARGET)-gcc
LD := $(TARGET)-ld
AS := nasm

# Emulator and boot-image tools.
QEMU := qemu-system-i386
GRUB_FILE := grub-file
GRUB_MKRESCUE := grub-mkrescue

# Project directories.
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir

# Generated artifacts.
BOOT_OBJECT := $(BUILD_DIR)/boot.o
KERNEL_OBJECT := $(BUILD_DIR)/kernel.o
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO_IMAGE := $(BUILD_DIR)/construction-os.iso
TERMINAL_OBJECT := $(BUILD_DIR)/terminal.o

# Source files.
BOOT_SOURCE := src/arch/i386/boot.asm
KERNEL_SOURCE := src/kernel/kernel.c
LINKER_SCRIPT := linker.ld
GRUB_CONFIG := grub/grub.cfg

# Freestanding C compilation options.
CFLAGS := \
	-std=c17 \
	-ffreestanding \
	-fno-stack-protector \
	-fno-pie \
	-fno-pic \
	-Wall \
	-Wextra \
	-Wpedantic \
	-O2 \
	-Iinclude

# Targets that represent actions instead of real files.
.PHONY: all check iso run clean

# Build the bootable ISO by default.
all: iso

# Assemble the Multiboot entry point.
$(BOOT_OBJECT): $(BOOT_SOURCE)
	mkdir -p $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

# Compile the freestanding C kernel.
$(KERNEL_OBJECT): $(KERNEL_SOURCE) include/kernel/terminal.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TERMINAL_OBJECT): src/kernel/terminal.c include/kernel/terminal.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link all objects into the final ELF kernel.
$(KERNEL_ELF): $(BOOT_OBJECT) $(KERNEL_OBJECT) $(TERMINAL_OBJECT) $(LINKER_SCRIPT)
	$(LD) \
		-T $(LINKER_SCRIPT) \
		-o $@ \
		$(BOOT_OBJECT) \
		$(KERNEL_OBJECT) \
		$(TERMINAL_OBJECT)

# Verify that GRUB recognizes the ELF as a Multiboot kernel.
check: $(KERNEL_ELF)
	$(GRUB_FILE) --is-x86-multiboot $(KERNEL_ELF)
	@echo "Multiboot validation succeeded."

# Construct the bootable ISO filesystem and image.
$(ISO_IMAGE): $(KERNEL_ELF) $(GRUB_CONFIG)
	$(GRUB_FILE) --is-x86-multiboot $(KERNEL_ELF)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_CONFIG) $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

iso: $(ISO_IMAGE)

# Start the bootable image in an emulated i386 computer.
run: $(ISO_IMAGE)
	$(QEMU) \
		-cdrom $(ISO_IMAGE) \
		-m 128M \
		-display gtk \
		-no-reboot \
		-no-shutdown

# Remove all generated artifacts.
clean:
	rm -rf $(BUILD_DIR)
