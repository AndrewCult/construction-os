# Architecture

This document describes the architecture of construction-os as it evolves.
It focuses on execution flow, subsystem responsibilities, and the reasoning
behind the main design decisions.

## Current scope

The kernel currently targets the 32-bit x86 architecture and is loaded by a
Multiboot-compatible bootloader. It runs in protected mode and provides basic
VGA text output, serial diagnostics, a Global Descriptor Table, an
Interrupt Descriptor Table with initial support for the division-error
exception, and remapped 8259 PIC controllers with all hardware IRQ lines
initially masked.

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
8. The Interrupt Descriptor Table is constructed and loaded.
9. The 8259 PIC controllers are remapped to IDT vectors 32-47, with every hardware IRQ line initially masked.
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
2. configures vector `0` with `idt_set_gate()`;
3. associates it with `isr_divide_error`;
4. loads the table into the CPU with the `lidt` instruction.

Each IDT entry stores the 32-bit handler address in two 16-bit fields,
`offset_low` and `offset_high`. It also contains the kernel code-segment
selector and the attributes that identify the entry as a present,
kernel-level 32-bit interrupt gate.

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

## Division-error exception

The CPU raises the division-error exception (`#DE`, vector `0`) when a division
uses a zero divisor or produces a result that cannot fit in the destination.

The current handling path is:

1. the CPU obtains the entry point from IDT vector `0`;
2. `isr_divide_error` disables interrupts and saves the general-purpose
   registers;
3. the assembly stub calls `exception_divide_error_handler()`;
4. the C handler reports the exception through VGA and the serial port;
5. the CPU is halted deliberately because execution cannot yet resume safely.

The complete path has been verified both with a software invocation of vector
`0` and with a real `div` instruction using a zero divisor.

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
