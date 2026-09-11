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

/*
 * Give legacy hardware enough time to process an I/O command.
 *
 * Port 0x80 was historically used for diagnostics. Writing to it is harmless
 * for our purposes and introduces the short delay required by older hardware.
 */
static void pic_io_wait(void) { outb(0x80u, 0u); }

void pic_initialize(void) {
  /*
   * Block every IRQ while the controllers are being reconfigured.
   */
  outb(PIC_MASTER_DATA_PORT, 0xFFu);
  outb(PIC_SLAVE_DATA_PORT, 0xFFu);

  /*
   * ICW1: begin initialization and announce that ICW4 will follow.
   */
  outb(PIC_MASTER_COMMAND_PORT, PIC_ICW1_INITIALIZE | PIC_ICW1_EXPECT_ICW4);
  pic_io_wait();

  outb(PIC_SLAVE_COMMAND_PORT, PIC_ICW1_INITIALIZE | PIC_ICW1_EXPECT_ICW4);
  pic_io_wait();

  /*
   * ICW2: map the master and slave IRQs to separate IDT ranges.
   */
  outb(PIC_MASTER_DATA_PORT, PIC_MASTER_VECTOR_OFFSET);
  pic_io_wait();

  outb(PIC_SLAVE_DATA_PORT, PIC_SLAVE_VECTOR_OFFSET);
  pic_io_wait();

  /*
   * ICW3: tell the master that the slave is connected to IRQ2,
   * and tell the slave its cascade identity.
   */
  outb(PIC_MASTER_DATA_PORT, (unsigned char)(1u << PIC_CASCADE_IRQ));
  pic_io_wait();

  outb(PIC_SLAVE_DATA_PORT, PIC_CASCADE_IRQ);
  pic_io_wait();

  /*
   * ICW4: select 8086-compatible interrupt handling mode.
   */
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

void pic_send_eoi(uint8_t irq) {
  if (irq >= PIC_IRQ_COUNT) {
    return;
  }

  if (irq >= PIC_SLAVE_FIRST_IRQ) {
    outb(PIC_SLAVE_COMMAND_PORT, PIC_EOI_COMMAND);
  }

  outb(PIC_MASTER_COMMAND_PORT, PIC_EOI_COMMAND);
}
