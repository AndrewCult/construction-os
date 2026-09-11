#include "arch/i386/idt.h"
#include "arch/i386/pic.h"
#include "boot/multiboot.h"
#include "kernel/serial.h"
#include "kernel/terminal.h"
#include <stdint.h>

/*
 * Write a 32-bit unsigned integer in decimal form to both diagnostic outputs.
 */
static void write_uint32_decimal(uint32_t value) {
  char digits[11];
  unsigned int length = 0u;

  /*
   * Extract digits from right to left.
   *
   * The do-while is necessary because the value zero must still produce one
   * digit.
   */
  do {
    digits[length] = (char)('0' + (value % 10u));
    ++length;
    value /= 10u;
  } while (value != 0u);

  /*
   * The digits were generated in reverse order, so reverse the buffer.
   */
  for (unsigned int left = 0u, right = length - 1u; left < right;
       ++left, --right) {
    const char temporary = digits[left];
    digits[left] = digits[right];
    digits[right] = temporary;
  }

  digits[length] = '\0';

  terminal_write(digits);
  serial_write(digits);
}

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
  const struct multiboot_info_prefix *multiboot_info =
      (const struct multiboot_info_prefix *)(uintptr_t)multiboot_info_address;

  if ((multiboot_info->flags & MULTIBOOT_INFO_MEMORY) != 0u) {
    terminal_write("Multiboot basic memory information is available.\n");
    serial_write("Multiboot basic memory information is available.\n");

    terminal_write("Lower memory: ");
    serial_write("Lower memory: ");
    write_uint32_decimal(multiboot_info->mem_lower);
    terminal_write(" KiB\n");
    serial_write(" KiB\n");

    terminal_write("Upper memory: ");
    serial_write("Upper memory: ");
    write_uint32_decimal(multiboot_info->mem_upper);
    terminal_write(" KiB\n");
    serial_write(" KiB\n");
  } else {
    terminal_write("Multiboot basic memory information is unavailable.\n");
    serial_write("Multiboot basic memory information is unavailable.\n");
  }

  idt_initialize();
  pic_initialize();

  terminal_write("Hello from construction-os!\n");
  terminal_write("The kernel is running in 32-bit protected mode.\n");
  terminal_write("Multiboot information received successfully.\n");

  serial_write("Hello from construction-os!\n");
  serial_write("The kernel is running in 32-bit protected mode.\n");
  serial_write("Multiboot information received successfully.\n");
}
