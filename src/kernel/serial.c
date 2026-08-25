#include "kernel/serial.h"
#include "arch/i386/io.h"

/* Base I/O port of the first serial interface */
#define COM1_PORT 0x3F8u

/*
 * Return non-zero when COM1 is ready to transmit another byte.
 */
static int serial_transmit_ready(void) {
  return (inb(COM1_PORT + 5u) & 0x20u) != 0u;
}

/*
 * Wait until the transmitter is ready, then send one character.
 */
static void serial_putchar(char character) {
  while (!serial_transmit_ready()) {
  }

  outb(COM1_PORT, (unsigned char)character);
}

void serial_initialize(void) {
  /* Disable serial-port interrupts during configuration. */
  outb(COM1_PORT + 1u, 0x00u);

  /* Enable access to the baud-rate divisor registers. */
  outb(COM1_PORT + 3u, 0x80u);

  /*
   * Configure 8 data bits, one stop bit and no parity.
   * This also disables access to the divisor registers.
   */
  outb(COM1_PORT + 3u, 0x03u);

  /* Enable the RTS and DTR modem-control signals. */
  outb(COM1_PORT + 4u, 0x03u);
}

void serial_write(const char *text) {
  for (unsigned int index = 0u; text[index] != '\0'; ++index) {
    serial_putchar(text[index]);
  }
}
