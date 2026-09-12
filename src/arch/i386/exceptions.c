#include "arch/i386/exceptions.h"
#include "kernel/serial.h"
#include "kernel/terminal.h"
#include <stdint.h>

/*
 * Human-readable names for the 32 exception vectors in our i686 model.
 * Vectors not assigned by the baseline architecture remain reserved.
 */
static const char *const exception_names[CPU_EXCEPTION_COUNT] = {
    "Division Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

/*
 * Write the same diagnostic text to VGA and the serial port.
 */
static void diagnostic_write(const char *text) {
  terminal_write(text);
  serial_write(text);
}

/*
 * Write a 32-bit value as eight hexadecimal digits.
 */
static void diagnostic_write_hex32(uint32_t value) {
  static const char hexadecimal_digits[] = "0123456789ABCDEF";
  char buffer[11];

  buffer[0] = '0';
  buffer[1] = 'x';

  for (unsigned int index = 0u; index < 8u; ++index) {
    const unsigned int shift = (7u - index) * 4u;
    const unsigned int digit = (value >> shift) & 0x0Fu;

    buffer[index + 2u] = hexadecimal_digits[digit];
  }

  buffer[10] = '\0';
  diagnostic_write(buffer);
}

/*
 * Report a fatal CPU exception through both diagnostic channels.
 */
void exception_handler(const struct exception_frame *frame) {
  diagnostic_write("\nCPU EXCEPTION\n");

  diagnostic_write("Name: ");
  if (frame->vector < CPU_EXCEPTION_COUNT) {
    diagnostic_write(exception_names[frame->vector]);
  } else {
    diagnostic_write("Unknown");
  }
  diagnostic_write("\n");

  diagnostic_write("Vector: ");
  diagnostic_write_hex32(frame->vector);
  diagnostic_write("\n");

  diagnostic_write("Error code: ");
  diagnostic_write_hex32(frame->error_code);
  diagnostic_write("\n");

  diagnostic_write("Instruction pointer: ");
  diagnostic_write_hex32(frame->instruction_pointer);
  diagnostic_write("\n");

  /*
   * Exception recovery is not implemented yet.
   */
  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}
