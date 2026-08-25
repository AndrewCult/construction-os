#ifndef ARCH_I386_IO_H
#define ARCH_I386_IO_H

/*
 * Write one byte to an x86 I/O port.
 */
static inline void outb(unsigned short port, unsigned char value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/*
 * Read one byte from an x86 I/O port.
 */
static inline unsigned char inb(unsigned short port) {
  unsigned char value;

  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));

  return value;
}

#endif
