bits 32

; Multiboot 1 header recognized by GRUB.
section .multiboot
align 4

multiboot_magic     equ 0x1BADB002
multiboot_flags     equ 0x00000003
multiboot_checksum  equ -(multiboot_magic + multiboot_flags)

dd multiboot_magic
dd multiboot_flags
dd multiboot_checksum

; Reserve a 16 KiB stack without storing it in the kernel image.
section .bss
align 16

stack_bottom:
    resb 16384
stack_top:

section .text

global _start
extern kernel_main

; Multiboot entry point. GRUB transfers control here in 32-bit mode.
_start:
    mov esp, stack_top        ; The x86 stack grows toward lower addresses.

    call kernel_main          ; Continue initialization in C.

; A kernel cannot return to a calling operating system.
.hang:
    cli                      ; No interrupt descriptor table exists yet.
    hlt                      ; Sleep instead of continuously consuming CPU.
    jmp .hang                ; Handle any unexpected wake-up safely.
