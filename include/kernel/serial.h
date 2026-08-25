#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

/*
 * Configure the first serial port (COM1)
 */
void serial_initialize(void);

/*
 * Write a null-terminated string to COM1.
 */
void serial_write(const char *text);

#endif
