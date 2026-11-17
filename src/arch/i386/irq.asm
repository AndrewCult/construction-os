bits 32

section .text
global irq0_stub
extern irq0_handler

irq0_stub:
    pushad
    cld
    mov ebx, esp
    and esp, -16
    call irq0_handler
    mov esp, ebx
    popad
    iretd
