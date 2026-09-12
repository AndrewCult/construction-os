# Building an Operating System

An educational operating system built from scratch for the 32-bit x86
architecture. The project begins with a small text-based system and aims to
grow, step by step, toward a lightweight retro-style graphical environment.

The main purpose is not to create a replacement for an existing operating
system. It is to understand how one works at the lowest practical level: from
boot and CPU initialization to memory, interrupts, processes, filesystems,
drivers, programs, and package management.

> [!IMPORTANT]
> This project is at an early learning and experimentation stage. It is not
> currently suitable for storing important data or for general-purpose use.

## Project goals

- Build a freestanding kernel primarily in C, with small and documented
  assembly components where required.
- Target the 32-bit x86 (`i686`) architecture.
- Learn each subsystem by implementing a minimal version before extending it.
- Start with a text interface and later explore a retro windowing environment.
- Design a simple native package format and package manager.
- Develop and debug safely in QEMU before testing on real hardware.
- Explore support for older, resource-constrained computers where practical.
- Keep the architecture and learning process documented as the project evolves.

## Initial scope

The first major milestone is a bootable system image that can:

1. load an `i686` kernel;
2. write diagnostic output to the screen and serial port;
3. handle basic CPU exceptions, timer interrupts, and keyboard input;
4. provide a minimal shell and filesystem;
5. load a simple program;
6. install a basic local package.

Hardware support beyond the emulated reference platform will be added
incrementally. Laptop-specific features such as Wi-Fi, battery management,
suspend/resume, audio, and accelerated graphics are not part of the initial
scope.

## Planned toolchain

| Component                 | Planned choice                      |
| ------------------------- | ----------------------------------- |
| Main language             | C17, freestanding subset            |
| Low-level code            | x86 assembly with NASM              |
| Target                    | `i686-elf`                          |
| Compiler and binary tools | GCC cross-compiler and GNU Binutils |
| Build system              | GNU Make                            |
| Bootloader                | GRUB with Multiboot                 |
| Kernel format             | ELF                                 |
| Emulator                  | QEMU                                |
| Debugger                  | GDB                                 |

These choices may evolve as the project exposes new requirements. Important
architectural decisions will be documented rather than changed silently.

## Architecture documentation

For a description of the current boot flow, subsystem responsibilities, and
exception-handling path, see [Architecture](docs/architecture.md).

## Roadmap

- [x] Establish and verify the cross-development environment
- [x] Boot the first kernel in QEMU
- [x] Add VGA text and serial output
- [x] Implement CPU exception handling
- [ ] Implement hardware interrupts
- [ ] Add a timer and keyboard input
- [ ] Introduce physical and virtual memory management
- [ ] Add processes, user mode, and system calls
- [ ] Implement storage and a minimal filesystem
- [ ] Build a shell and load user programs
- [ ] Add a network stack and a minimal text-mode network client
- [ ] Design the package format and package manager
- [ ] Test on selected legacy hardware
- [ ] Explore a text UI and, later, a graphical window system

The roadmap describes direction, not a fixed release schedule. Each item will
be divided into small, testable milestones as development progresses.

CPU exception handling is now implemented for all 32 architecture-reserved
vectors. The current milestone is the hardware-interrupt foundation. The
kernel remaps the two 8259 PIC controllers to vectors 32–47 while keeping
every IRQ line masked. The next step is to install the IRQ0 handler and
configure the PIT without enabling unrelated hardware interrupts.

## Build and run

The build requires an `i686-elf` GCC/Binutils cross-toolchain, NASM, GRUB
utilities (including `grub-file` and `grub-mkrescue`), `xorriso`, GNU Make,
and QEMU for i386.

From the repository root:

```sh
make check  # Build the ELF kernel and validate its Multiboot header.
make iso    # Build build/construction-os.iso.
make run    # Build the ISO and start it in QEMU.
make clean  # Remove generated artifacts.
```

`make` is equivalent to `make iso`. While QEMU is running, VGA output appears
in its display window and serial diagnostics are written to the terminal that
started QEMU.

## Repository structure

| Path             | Purpose                                         |
| ---------------- | ----------------------------------------------- |
| `src/arch/i386/` | x86 boot, descriptor-table, and exception code  |
| `src/kernel/`    | architecture-independent kernel facilities      |
| `include/`       | public headers, arranged like the source tree   |
| `grub/`          | GRUB configuration used in the bootable image   |
| `experiments/`   | small, isolated exercises used during learning  |
| `docs/`          | Current architecture and execution flow         |
| `linker.ld`      | kernel memory layout and ELF linking rules      |
| `Makefile`       | kernel, ISO, validation, and QEMU build targets |

## Repository status

The repository currently builds a bootable 32-bit Multiboot kernel and ISO.
GRUB transfers control to an assembly entry point that preserves the Multiboot
magic value and information address, installs a minimal flat GDT, reloads the
segment registers, prepares an aligned kernel stack, and passes the Multiboot
values to the C kernel.

The kernel provides VGA and serial diagnostic output and installs IDT entries
for all 32 CPU exception vectors. Assembly entry stubs normalize the stack
layout for exceptions with and without a CPU-provided error code, allowing a
shared C handler to report the exception name, vector, error code, and
instruction pointer before halting the processor safely.

The exception path has been verified with an Invalid Opcode exception (`#UD`)
without a CPU-provided error code and a General Protection Fault (`#GP`) with
a CPU-provided error code.

The two 8259 PIC controllers are initialized during kernel startup and remapped
away from the CPU exception range. All IRQ lines remain masked until their
handlers are installed and tested individually.

During startup, the kernel validates the Multiboot magic value and reads the
basic lower and upper memory information when GRUB marks those fields as
available. Detailed memory-map parsing and physical memory management are not
implemented yet.

## Conventions

- Source code, identifiers, comments, logs, and error messages use English.
- Commit messages, issue titles, and technical terminology use English.
- Project documentation uses English and will later be expanded under `docs/`.

## Contributing

The project is currently developed as a personal learning project. Suggestions,
technical corrections, and educational references are welcome through GitHub
issues. Contribution guidelines will be added when the codebase is ready for
external contributions.

## License

This project is distributed under the [MIT License](LICENSE).
