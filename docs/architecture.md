# Architecture

This document describes the architecture of construction-os as it evolves.
It focuses on execution flow, subsystem responsibilities, and the reasoning
behind the main design decisions.

## Current scope

The kernel currently targets the 32-bit x86 architecture and is loaded by a
Multiboot-compatible bootloader. It runs in protected mode and provides basic
VGA text output, serial diagnostics, a Global Descriptor Table, and an
Interrupt Descriptor Table with initial support for the division-error
exception.

## Boot flow

The current boot sequence is:

1. GRUB loads the kernel through the Multiboot interface.
2. `_start` installs the Global Descriptor Table.
3. The assembly entry point initializes the kernel stack.
4. Control is transferred to `kernel_main()`.
5. The kernel initializes VGA text output and the serial port.
6. The Interrupt Descriptor Table is constructed and loaded.
7. The kernel enters its normal execution state.

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
