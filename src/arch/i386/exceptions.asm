bits 32

section .text

global isr_divide_error
extern exception_divide_error_handler

isr_divide_error:
    cli
    pushad

    call exception_divide_error_handler

    popad
    iretd
