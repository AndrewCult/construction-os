#ifndef KERNEL_TERMINAL_H
#define KERNEL_TERMINAL_H

/*
 * Prepare the VGA text terminal and clear the screen.
 */
void terminal_initialize(void);

/*
 * Write a null-terminated string at the current cursor position.
 */
void terminal_write(const char *text);

#endif
