# Architecture

This document describes the architecture of construction-os as it evolves.
It focuses on execution flow, subsystem responsibilities, and the reasoning
behind the main design decisions.

## Current scope

The kernel currently targets the 32-bit x86 architecture and is loaded by a
Multiboot-compatible bootloader. It runs in protected mode and provides basic
VGA text output, serial diagnostics, a Global Descriptor Table, an Interrupt
Descriptor Table for the 32 CPU exception vectors, IRQ0, and IRQ1, and two remapped
8259 PICs. A boot check uses the Programmable Interval Timer (PIT) to verify
repeated hardware interrupt delivery. The kernel then enables PS/2 keyboard
input and decodes scan codes in its main loop.

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
8. The kernel installs the 32 CPU exception stubs and both hardware IRQ stubs in the
   Interrupt Descriptor Table, then loads the table.
9. The 8259 PIC controllers are remapped to IDT vectors 32–47, with every
   hardware IRQ line initially masked.
10. The kernel programs PIT channel 0, unmasks IRQ0 and waits for at least
    two timer interrupts.
11. The kernel enables maskable interrupts, prints the startup messages and
    records the tick count. After 100 more ticks it prints a serial diagnostic
    and unmasks IRQ1.
12. The main loop consumes queued keyboard bytes and sleeps when the queue
    is empty. Both the timer and the keyboard can wake the CPU.

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
3. installs `irq0_stub` at `PIC_MASTER_VECTOR_OFFSET` (vector 32);
4. installs `irq1_stub` at the next vector (33);
5. loads the table into the CPU with the `lidt` instruction.

Each installed entry is a present, ring-0, 32-bit interrupt gate.

The exception entry points are stored in `exception_stub_table`. The table
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
6. leaves every IRQ masked when initialization completes.

The boot check unmasks only IRQ0. The PIC uses vector 32 to reach the timer
stub, while the kernel uses line number 0 to change the PIC mask and acknowledge
each timer interrupt. After the post-boot timer check, the kernel unmasks
line 1 so the master PIC can deliver keyboard interrupts on vector 33.

## Timer interrupts during boot

`pit_initialize()` configures channel 0 in mode 2 (rate generator). It writes
command `0x34` to port `0x43`, then sends the low and high bytes of divisor
11932 to port `0x40`. The timer repeats the count about 100 times per second.

With IRQ0 unmasked, `kernel_main()` checks `timer_ticks` with CPU interrupts
disabled. While the count is below two, it executes `sti` immediately followed
by `hlt`. This lets the CPU sleep until an interrupt arrives, including one
already pending when the wait begins.

On IRQ0, `irq0_stub` saves the general-purpose registers, clears the direction
flag and aligns the stack before calling `irq0_handler()`. The C handler
increments `timer_ticks` and calls `pic_send_eoi(0)` to acknowledge service on
the master PIC, allowing it to deliver the next tick. The stub restores the
stack and registers, then returns to the interrupted code with `iretd`.

After waking, the CPU executes `cli` before the loop reads the count again.
Once the count reaches at least two, the kernel enables maskable interrupts
with `sti` and prints its startup messages. It samples `timer_ticks` after the
final message and uses unsigned subtraction to count ticks since that sample,
including across a 32-bit counter wrap. While waiting, `hlt` lets the CPU
sleep until an interrupt arrives. At 100 or more elapsed ticks the kernel
prints one confirmation and enables keyboard input. IRQ0 remains unmasked.

## Keyboard interrupts and input processing

The NASM macro `IRQ_STUB` generates both hardware interrupt entry points.
Each expansion uses the same register-save and return sequence and calls the
C handler named in its second argument.

On IRQ1, `irq1_handler()` reads one byte from port `0x60` and appends it to a
ring buffer. The read and write indices are equal when the queue is empty.
Advancing the write index onto the read index would make a full queue appear
empty, so one of the four slots stays unused. If three bytes are already
queued, the handler discards the incoming byte and increments
`keyboard_dropped_bytes`. It acknowledges the master PIC in either case.

The main loop disables maskable interrupts with `cli` before checking the
queue. If it is empty, adjacent `sti` and `hlt` instructions let the CPU sleep
without missing a wake-up between the check and the halt. Otherwise the loop
removes one byte and snapshots the drop count before enabling interrupts
again. It can then write diagnostics and decode that byte while IRQ1 can
enqueue more input. A change in the drop count is reported on VGA and serial.

The decoder interprets scan-code set 1 bytes. It remembers an `0xE0` prefix
across loop iterations and recognizes the following arrow codes. For these
keys, setting bit 7 of the second byte indicates release. Num Lock uses a
single byte. The loop logs each dequeued byte on the serial port and prints
a key event when the decoder recognizes a press or release.

| Key | Press bytes (hex) | Release bytes (hex) |
| --- | --- | --- |
| Right arrow | `E0 4D` | `E0 CD` |
| Left arrow | `E0 4B` | `E0 CB` |
| Up arrow | `E0 48` | `E0 C8` |
| Down arrow | `E0 50` | `E0 D0` |
| Num Lock | `45` | `C5` |

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
