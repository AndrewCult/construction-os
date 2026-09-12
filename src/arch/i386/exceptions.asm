bits 32

section .text

extern exception_handler
global exception_stub_table


; Exceptions for which the CPU does not push an error code.
;
; Push a synthetic zero so that every exception produces the same
; stack layout before entering exception_common.

%macro EXCEPTION_NO_ERROR_CODE 1
exception_%1:
    cli
    push dword 0
    push dword %1
    jmp exception_common
%endmacro

; Exceptions for which the CPU has already pushed an error code.
;
; Only the vector number must be added by the stub.

%macro EXCEPTION_ERROR_CODE 1
exception_%1:
    cli
    push dword %1
    jmp exception_common
%endmacro

EXCEPTION_NO_ERROR_CODE 0
EXCEPTION_NO_ERROR_CODE 1
EXCEPTION_NO_ERROR_CODE 2
EXCEPTION_NO_ERROR_CODE 3
EXCEPTION_NO_ERROR_CODE 4
EXCEPTION_NO_ERROR_CODE 5
EXCEPTION_NO_ERROR_CODE 6
EXCEPTION_NO_ERROR_CODE 7
EXCEPTION_ERROR_CODE    8
EXCEPTION_NO_ERROR_CODE 9
EXCEPTION_ERROR_CODE    10
EXCEPTION_ERROR_CODE    11
EXCEPTION_ERROR_CODE    12
EXCEPTION_ERROR_CODE    13
EXCEPTION_ERROR_CODE    14
EXCEPTION_NO_ERROR_CODE 15
EXCEPTION_NO_ERROR_CODE 16
EXCEPTION_ERROR_CODE    17
EXCEPTION_NO_ERROR_CODE 18
EXCEPTION_NO_ERROR_CODE 19
EXCEPTION_NO_ERROR_CODE 20
EXCEPTION_NO_ERROR_CODE 21
EXCEPTION_NO_ERROR_CODE 22
EXCEPTION_NO_ERROR_CODE 23
EXCEPTION_NO_ERROR_CODE 24
EXCEPTION_NO_ERROR_CODE 25
EXCEPTION_NO_ERROR_CODE 26
EXCEPTION_NO_ERROR_CODE 27
EXCEPTION_NO_ERROR_CODE 28
EXCEPTION_NO_ERROR_CODE 29
EXCEPTION_NO_ERROR_CODE 30
EXCEPTION_NO_ERROR_CODE 31


; Common entry point shared by every CPU exception.

exception_common:
    pushad

     ; Pass the current stack pointer as the first C argument.
     ; It points to the beginning of struct exception_frame.

    push esp
    call exception_handler
    add esp, 4

; These instructions allow the stub to return correctly if the
; C handler becomes recoverable in the future. The current handler
; deliberately never returns.

    popad
    add esp, 8
    iretd

section .rodata
align 4

; Table of assembly entry points consumed by idt_initialize().

exception_stub_table:
    dd exception_0
    dd exception_1
    dd exception_2
    dd exception_3
    dd exception_4
    dd exception_5
    dd exception_6
    dd exception_7
    dd exception_8
    dd exception_9
    dd exception_10
    dd exception_11
    dd exception_12
    dd exception_13
    dd exception_14
    dd exception_15
    dd exception_16
    dd exception_17
    dd exception_18
    dd exception_19
    dd exception_20
    dd exception_21
    dd exception_22
    dd exception_23
    dd exception_24
    dd exception_25
    dd exception_26
    dd exception_27
    dd exception_28
    dd exception_29
    dd exception_30
    dd exception_31
