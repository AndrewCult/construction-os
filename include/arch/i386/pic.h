#ifndef ARCH_I386_PIC_H
#define ARCH_I386_PIC_H

#include <stdint.h>

/*
 * IDT vector ranges assigned to the two 8259 PIC controllers.
 */
#define PIC_MASTER_VECTOR_OFFSET 32u
#define PIC_SLAVE_VECTOR_OFFSET 40u

/*
 * Remap the PIC interrupt vectors and initially mask every IRQ line.
 */
void pic_initialize(void);

/*
 * Enable or disable and individual IRQ line.
 */
void pic_unmask_irq(uint8_t irq);
void pic_mask_irq(uint8_t irq);

/*
 * Notify the PIC that the specified IRQ has been handled.
 */
void pic_send_eoi(uint8_t irq);

#endif
