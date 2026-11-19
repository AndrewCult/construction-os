#include "arch/i386/pit.h"
#include "arch/i386/io.h"
#include <stdint.h>

#define PIT_CHANNEL0_DATA_PORT 0x40u
#define PIT_COMMAND_PORT 0x43u

#define PIT_CHANNEL0_RATE_GENERATOR 0x34u
#define PIT_DIVISOR_100_HZ 11932u

void pit_initialize(void) {
  const uint16_t divisor = PIT_DIVISOR_100_HZ;

  outb(PIT_COMMAND_PORT, PIT_CHANNEL0_RATE_GENERATOR);

  outb(PIT_CHANNEL0_DATA_PORT, (uint8_t)(divisor & 0xFFu));
  outb(PIT_CHANNEL0_DATA_PORT, (uint8_t)(divisor >> 8));
}
