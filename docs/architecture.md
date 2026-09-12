# Architecture

This document describes the architecture of construction-os as it evolves.
It focuses on execution flow, subsystem responsibilities, and the reasoning
behind the main design decisions.

## Current scope

The kernel currently targets the 32-bit x86 architecture and is loaded by a
Multiboot-compatible bootloader. It runs in protected mode and provides basic
VGA text output, serial diagnostics, a Global Descriptor Table, an Interrupt
Descriptor Table covering all 32 CPU exception vectors, and remapped 8259 PIC
controllers with all hardware IRQ lines initially masked.

## Boot flow

The current boot sequence is:

1. GRUB loads the kernel through the Multiboot interface.
2. `_start` preserves the Multiboot magic value and information address.
3. `_start` installs the Global Descriptor Table.
4. The assembly entry point initializes and aligns the kernel stack.
5. The Multiboot values are passed to `kernel_main()`.
6. The kernel initializes VGA text output and the serial port.
7. The kernel validates the Multiboot handoff and reads the available basic
   memory information.
8. The Interrupt Descriptor Table is populated with the 32 CPU exception
   entry points and loaded.
9. The 8259 PIC controllers are remapped to IDT vectors 32–47, with every
   hardware IRQ line initially masked.
10. The kernel enters its normal execution state.

## Multiboot handoff

When GRUB transfers control to `_start`, it provides two values through CPU
registers:

- `EAX` contains the Multiboot bootloader magic value;
- `EBX` contains the physical address of the Multiboot information structure.

The assembly entry point preserves these values before reloading the segment
registers. After preparing the kernel stack, it passes them to `kernel_main()`
using the i386 C calling convention.

The kernel validates the magic value before interpreting the information
structure. It then checks bit 0 of the Multiboot `flags` field. When this bit is
set, `mem_lower` and `mem_upper` contain valid basic memory information,
expressed in kibibytes.

These two values provide only a summary of available memory. They are useful
for validating the bootloader handoff, but they are not sufficient for physical
memory allocation because they do not describe reserved regions or holes. The
kernel will use the detailed Multiboot memory map for that purpose.

## Global Descriptor Table

The Global Descriptor Table provides the flat memory model used by the kernel
in 32-bit protected mode.

The current GDT contains:

- a mandatory null descriptor;
- a kernel code segment;
- a kernel data segment.

Both usable segments cover the 32-bit address space. Segmentation is therefore
used to satisfy the protected-mode architecture, while memory is treated as a
flat address space.

## Interrupt Descriptor Table

The Interrupt Descriptor Table associates interrupt vectors with their entry
points.

During initialization, `idt_initialize()`:

1. prepares the IDT descriptor;
2. installs the assembly entry points for CPU exception vectors 0–31;
3. configures each entry as a present, ring-0, 32-bit interrupt gate;
4. loads the table into the CPU with the `lidt` instruction.

The assembly entry points are stored in `exception_stub_table`. The table
allows the C initialization code to associate each exception vector with its
corresponding stub without declaring and installing every entry separately.

Each IDT entry divides the 32-bit entry-point address between the 16-bit
`offset_low` and `offset_high` fields. It also contains the kernel code-segment
selector and the attributes that describe the interrupt gate.

## Programmable Interrupt Controller

The system uses the two cascaded 8259 Programmable Interrupt Controllers
provided by the legacy i386 platform. The master handles IRQ0–IRQ7, while the
slave handles IRQ8–IRQ15 and reaches the CPU through IRQ2 on the master.

During initialization, `pic_initialize()`:

1. masks every hardware IRQ line;
2. remaps the master PIC to IDT vectors 32–39;
3. remaps the slave PIC to IDT vectors 40–47;
4. configures the cascade connection through IRQ2;
5. selects 8086-compatible operation;
6. leaves every IRQ masked until a corresponding IDT handler is installed.

The PIC module also provides operations to mask or unmask an individual IRQ
line and to send an End of Interrupt notification after an IRQ has been
handled. CPU interrupts remain globally disabled at the current stage.

## CPU exceptions

The first 32 IDT vectors are reserved by the x86 architecture for CPU
exceptions. Each vector is associated with a small assembly entry stub before
execution reaches the shared C handler.

The processor automatically pushes the interrupted instruction pointer, code
segment, and flags. For some exceptions in the baseline i686 architecture, it
also pushes an error code. These exceptions use vectors 8, 10, 11, 12, 13, 14,
and 17.

The exception entry stubs normalize the two possible stack layouts:

- when the CPU does not provide an error code, the stub pushes a synthetic
  zero;
- when the CPU provides an error code, the stub preserves that value;
- every stub pushes its exception vector;
- the common assembly entry point saves the general-purpose registers and
  passes the resulting `exception_frame` to C.

This convention gives `exception_handler()` a uniform representation of every
exception. The handler reports the exception name, vector, error code, and
instruction pointer through both VGA and the serial port.

Exception recovery is not implemented yet. After reporting an exception, the
handler disables interrupts and deliberately halts the processor so that a
fatal fault cannot continue with potentially corrupted state.

The common path has been tested with:

- an Invalid Opcode exception (`#UD`, vector 6), using the `UD2` instruction
  and a synthetic zero error code;
- a General Protection Fault (`#GP`, vector 13), produced by loading a
  nonexistent GDT selector and carrying a CPU-provided error code.

The earlier division-error path was also tested with a real division by zero.

## Diagnostic output

VGA text output provides immediate information on the virtual machine display.
Serial output provides the same essential diagnostics through QEMU's standard
output, making early kernel failures easier to observe and record.

## Design principles

The project currently follows these principles:

- architecture-specific code remains under `src/arch/i386`;
- `kernel_main()` initializes subsystems without knowing their internal
  representation;
- assembly is limited to operations that require direct CPU interaction;
- C contains the higher-level kernel behavior;
- each new mechanism is tested in isolation before becoming part of normal
  startup.
