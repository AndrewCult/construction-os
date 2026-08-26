#include "kernel/serial.h"
#include "kernel/terminal.h"

/*
 * Handle the x86 division-error exception.
 *
 * Execution cannot safely continue yet, so stop the CPU after reporting
 * the failure through both available diagnostic channels.
 */
void exception_divide_error_handler(void) {
  terminal_write("\nEXCEPTION: Division error.\n");
  serial_write("EXCEPTION: Division error.\n");

  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}
