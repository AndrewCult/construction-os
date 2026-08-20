#include "kernel/terminal.h"

void kernel_main(void) {
  terminal_initialize();

  terminal_write("Hello from construction-os!\n");
  terminal_write("The kernel is running in 32-bit protected mode.");
}
