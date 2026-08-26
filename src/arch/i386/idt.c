#include "arch/i386/idt.h"

/* An x86 IDT contains 256 entries. */
#define IDT_ENTRY_COUNT 256u

/*
 * One entry in the 32-bit x86 Interrupt Descriptor Table.
 */
struct idt_entry {
  unsigned short offset_low;
  unsigned short selector;
  unsigned char zero;
  unsigned char type_attributes;
  unsigned short offset_high;
} __attribute__((packed));

/*
 * Structure consumed by the lidt instruction.
 */
struct idt_pointer {
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));

/* Zero-initialized automatically because it has static storage duration. */
static struct idt_entry idt_entries[IDT_ENTRY_COUNT];

/* Descriptor passed to the CPU by idt_load(). */
static struct idt_pointer idt_descriptor;

/* Implemented in src/arch/i386/idt_load.asm. */
extern void idt_load(const struct idt_pointer *descriptor);

void idt_initialize(void) {
  idt_descriptor.limit = (unsigned short)(sizeof(idt_entries) - 1u);
  idt_descriptor.base = (unsigned int)idt_entries;

  idt_load(&idt_descriptor);
}
