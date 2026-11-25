#include "arch/i386/idt.h"
#include "arch/i386/exceptions.h"
#include "arch/i386/pic.h"

#define IDT_ENTRY_COUNT 256u

extern void irq0_stub(void);
extern void irq1_stub(void);

/* Hardware layouts for an IDT gate and the descriptor read by LIDT. */
struct idt_entry {
  unsigned short offset_low;
  unsigned short selector;
  unsigned char zero;
  unsigned char type_attributes;
  unsigned short offset_high;
} __attribute__((packed));

struct idt_pointer {
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));

/* Zero-initialized entries stay non-present until a gate is installed. */
static struct idt_entry idt_entries[IDT_ENTRY_COUNT];

static struct idt_pointer idt_descriptor;

/* Implemented in src/arch/i386/idt_load.asm. */
extern void idt_load(const struct idt_pointer *descriptor);

/* Table of exception entry points defined in exceptions.asm. */
extern void (*const exception_stub_table[CPU_EXCEPTION_COUNT])(void);

/* Configure an IDT interrupt gate for the specified vector and handler. */
static void idt_set_gate(unsigned char vector, void (*handler)(void)) {
  unsigned int address = (unsigned int)handler;

  idt_entries[vector].offset_low = (unsigned short)(address & 0xFFFFu);
  idt_entries[vector].selector = 0x08u; /* gdt_code in boot.asm. */
  idt_entries[vector].zero = 0u;
  idt_entries[vector].type_attributes =
      0x8Eu; /* Present, DPL 0, 32-bit interrupt gate. */
  idt_entries[vector].offset_high = (unsigned short)((address >> 16) & 0xFFFFu);
}

void idt_initialize(void) {
  idt_descriptor.limit = (unsigned short)(sizeof(idt_entries) - 1u);
  idt_descriptor.base = (unsigned int)idt_entries;

  for (unsigned int vector = 0u; vector < CPU_EXCEPTION_COUNT; ++vector) {
    idt_set_gate((unsigned char)vector, exception_stub_table[vector]);
  }

  idt_set_gate(PIC_MASTER_VECTOR_OFFSET, irq0_stub);
  idt_set_gate(PIC_MASTER_VECTOR_OFFSET + 1u, irq1_stub);

  idt_load(&idt_descriptor);
}
