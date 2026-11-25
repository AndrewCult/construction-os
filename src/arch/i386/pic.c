#include "arch/i386/pic.h"
#include "arch/i386/io.h"
#include <stdint.h>

/* I/O ports used by the master and slave 8259 PIC controllers. */
#define PIC_MASTER_COMMAND_PORT 0x20u
#define PIC_MASTER_DATA_PORT 0x21u
#define PIC_SLAVE_COMMAND_PORT 0xA0u
#define PIC_SLAVE_DATA_PORT 0xA1u

/* Initialization and control values understood by the 8259 PIC. */
#define PIC_ICW1_INITIALIZE 0x10u
#define PIC_ICW1_EXPECT_ICW4 0x01u
#define PIC_ICW4_8086_MODE 0x01u
#define PIC_EOI_COMMAND 0x20u

#define PIC_IRQ_COUNT 16u
#define PIC_SLAVE_FIRST_IRQ 8u
#define PIC_CASCADE_IRQ 2u

/* A write to port 0x80 provides the I/O delay needed by older PICs. */
static void pic_io_wait(void) { outb(0x80u, 0u); }

void pic_initialize(void) {
  outb(PIC_MASTER_DATA_PORT, 0xFFu);
  outb(PIC_SLAVE_DATA_PORT, 0xFFu);

  /* Program both PICs in ICW1-ICW4 order. */
  outb(PIC_MASTER_COMMAND_PORT, PIC_ICW1_INITIALIZE | PIC_ICW1_EXPECT_ICW4);
  pic_io_wait();

  outb(PIC_SLAVE_COMMAND_PORT, PIC_ICW1_INITIALIZE | PIC_ICW1_EXPECT_ICW4);
  pic_io_wait();

  outb(PIC_MASTER_DATA_PORT, PIC_MASTER_VECTOR_OFFSET);
  pic_io_wait();

  outb(PIC_SLAVE_DATA_PORT, PIC_SLAVE_VECTOR_OFFSET);
  pic_io_wait();

  /* ICW3: the master takes a bitmask of slave inputs.
   * The slave takes its input number on the master. */
  outb(PIC_MASTER_DATA_PORT, (unsigned char)(1u << PIC_CASCADE_IRQ));
  pic_io_wait();

  outb(PIC_SLAVE_DATA_PORT, PIC_CASCADE_IRQ);
  pic_io_wait();

  outb(PIC_MASTER_DATA_PORT, PIC_ICW4_8086_MODE);
  pic_io_wait();

  outb(PIC_SLAVE_DATA_PORT, PIC_ICW4_8086_MODE);
  pic_io_wait();

  /*
   * Keep every IRQ masked until its IDT handler has been installed.
   */
  outb(PIC_MASTER_DATA_PORT, 0xFFu);
  outb(PIC_SLAVE_DATA_PORT, 0xFFu);
}

void pic_unmask_irq(uint8_t irq) {
  uint16_t data_port;
  uint8_t bit;
  uint8_t mask;

  if (irq >= PIC_IRQ_COUNT) {
    return;
  }

  if (irq < PIC_SLAVE_FIRST_IRQ) {
    data_port = PIC_MASTER_DATA_PORT;
    bit = irq;
  } else {
    data_port = PIC_SLAVE_DATA_PORT;
    bit = (uint8_t)(irq - PIC_SLAVE_FIRST_IRQ);
  }

  mask = inb(data_port);
  mask = (uint8_t)(mask & (uint8_t)~(1u << bit));
  outb(data_port, mask);

  /*
   * Slave IRQs reach the CPU through IRQ2 on the master PIC.
   */
  if (irq >= PIC_SLAVE_FIRST_IRQ) {
    mask = inb(PIC_MASTER_DATA_PORT);
    mask = (uint8_t)(mask & (uint8_t)~(1u << PIC_CASCADE_IRQ));
    outb(PIC_MASTER_DATA_PORT, mask);
  }
}

void pic_mask_irq(uint8_t irq) {
  uint16_t data_port;
  uint8_t bit;
  uint8_t mask;

  if (irq >= PIC_IRQ_COUNT) {
    return;
  }

  if (irq < PIC_SLAVE_FIRST_IRQ) {
    data_port = PIC_MASTER_DATA_PORT;
    bit = irq;
  } else {
    data_port = PIC_SLAVE_DATA_PORT;
    bit = (uint8_t)(irq - PIC_SLAVE_FIRST_IRQ);
  }

  mask = inb(data_port);
  mask = (uint8_t)(mask | (uint8_t)(1u << bit));
  outb(data_port, mask);
}

/* For a slave IRQ, acknowledge the slave before the master. */

void pic_send_eoi(uint8_t irq) {
  if (irq >= PIC_IRQ_COUNT) {
    return;
  }

  if (irq >= PIC_SLAVE_FIRST_IRQ) {
    outb(PIC_SLAVE_COMMAND_PORT, PIC_EOI_COMMAND);
  }

  outb(PIC_MASTER_COMMAND_PORT, PIC_EOI_COMMAND);
}
