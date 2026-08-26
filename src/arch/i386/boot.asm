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

; Minimal flat Global Descriptor Table.
section .rodata
align 8

gdt_start:

gdt_null:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b  ; 0x9A executable and readble
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b  ; 0x92 readble and writeable
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SELECTOR equ gdt_code - gdt_start
DATA_SELECTOR equ gdt_data - gdt_start

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
    cli

    lgdt [gdt_descriptor]

    jmp CODE_SELECTOR:.reload_cs

.reload_cs:
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, stack_top        ; The x86 stack grows toward lower addresses.

    call kernel_main          ; Continue initialization in C.

; A kernel cannot return to a calling operating system.
.hang:
    cli                      ; No interrupt descriptor table exists yet.
    hlt                      ; Sleep instead of continuously consuming CPU.
    jmp .hang                ; Handle any unexpected wake-up safely.
