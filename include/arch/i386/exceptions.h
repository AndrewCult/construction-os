#ifndef ARCH_I386_EXCEPTIONS_H
#define ARCH_I386_EXCEPTIONS_H

#include <stdint.h>

/* The first 32 IDT vectors are reserved for CPU exceptions. */
#define CPU_EXCEPTION_COUNT 32u

/*
 * CPU state prepared by the exception entry stubs.
 *
 * The first eight fields correspond to the values saved by pushad.
 * The vector and error_code fields are normalized by the assembly
 * stubs so that every CPU exception presents the same layout to C.
 */
struct exception_frame {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint32_t vector;
  uint32_t error_code;
  uint32_t instruction_pointer;
  uint32_t code_segment;
  uint32_t flags;
};

/*
 * Report a fatal CPU exception and halt execution.
 */
void exception_handler(const struct exception_frame *frame);

#endif
