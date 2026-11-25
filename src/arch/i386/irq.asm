bits 32

section .text

; %1 = assembly entry point installed in the IDT
; %2 = C handler called for this IRQ
%macro IRQ_STUB 2
    global %1
    extern %2

%1:
    pushad
    cld

    ; Keep the address of the saved registers in EBX while ESP is aligned
    ; for the C call. The handler preserves EBX, so POPAD can use that address.
    mov ebx, esp
    and esp, -16
    call %2
    mov esp, ebx
    popad
    iretd
%endmacro

IRQ_STUB irq0_stub, irq0_handler
IRQ_STUB irq1_stub, irq1_handler
