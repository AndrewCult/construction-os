[English](README.md) | [Italiano](README.it.md)

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
- Keep the architecture and learning process documented in English and Italian.

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

| Component | Planned choice |
| --- | --- |
| Main language | C17, freestanding subset |
| Low-level code | x86 assembly with NASM |
| Target | `i686-elf` |
| Compiler and binary tools | GCC cross-compiler and GNU Binutils |
| Build system | GNU Make |
| Bootloader | GRUB with Multiboot |
| Kernel format | ELF |
| Emulator | QEMU |
| Debugger | GDB |

These choices may evolve as the project exposes new requirements. Important
architectural decisions will be documented rather than changed silently.

## Roadmap

- [ ] Establish and verify the cross-development environment
- [ ] Boot the first kernel in QEMU
- [ ] Add VGA text and serial output
- [ ] Implement CPU exceptions and hardware interrupts
- [ ] Add a timer and keyboard input
- [ ] Introduce physical and virtual memory management
- [ ] Add processes, user mode, and system calls
- [ ] Implement storage and a minimal filesystem
- [ ] Build a shell and load user programs
- [ ] Design the package format and package manager
- [ ] Test on selected legacy hardware
- [ ] Explore a text UI and, later, a graphical window system

The roadmap describes direction, not a fixed release schedule. Each item will
be divided into small, testable milestones as development progresses.

## Repository status

The repository currently contains the project foundation and documentation.
Build instructions will be added with the first bootable kernel so that every
documented command can be tested and reproduced.

## Languages and conventions

Documentation is available in English and Italian. English is the canonical
language if translations temporarily differ.

- Source code, identifiers, comments, logs, and error messages use English.
- Commit messages, issue titles, and technical terminology use English.
- English documentation lives in `README.md` and, later, `docs/en/`.
- Italian documentation lives in `README.it.md` and, later, `docs/it/`.
- Corresponding documents should preserve the same structure in both languages.

## Contributing

The project is currently developed as a personal learning project. Suggestions,
technical corrections, and educational references are welcome through GitHub
issues. Contribution guidelines will be added when the codebase is ready for
external contributions.

## License

This project is distributed under the [MIT License](LICENSE).
