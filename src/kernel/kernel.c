#include "arch/i386/idt.h"
#include "kernel/serial.h"
#include "kernel/terminal.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u

void kernel_main(unsigned int multiboot_magic,
                 unsigned int multiboot_info_address) {
  terminal_initialize();
  serial_initialize();

  if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    terminal_write("ERROR: Invalid Multiboot magic value.\n");
    serial_write("ERROR: Invalid Multiboot magic value.\n");

    for (;;) {
      __asm__ volatile("cli; hlt");
    }
  }

  /*
   * The Multiboot information structure is not inspected yet.
   * Explicitly mark its address as intentionally unused.
   */
  (void)multiboot_info_address;

  idt_initialize();

  terminal_write("Hello from construction-os!\n");
  terminal_write("The kernel is running in 32-bit protected mode.\n");
  terminal_write("Multiboot information received successfully.\n");

  serial_write("Hello from construction-os!\n");
  serial_write("The kernel is running in 32-bit protected mode.\n");
  serial_write("Multiboot information received successfully.\n");
}
