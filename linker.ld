/* Produce a 32-bit ELF executable for the i386 architecture. */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

/* Begin execution at the assembly entry point. */
ENTRY(_start)

SECTIONS
{
    /*
     * Load the kernel at 1 MiB. The lower physical memory contains
     * firmware data, legacy structures, and memory-mapped devices.
     */
    . = 0x00100000;

    kernel_start = .;

    /*
     * GRUB must find the Multiboot header early in the kernel image.
     * KEEP prevents it from being removed as apparently unused data.
     */
    .multiboot ALIGN(4) :
    {
        KEEP(*(.multiboot))
    }

    /*
     * Executable machine code from every input object.
     * Page alignment keeps the region suitable for future paging.
     */
    .text ALIGN(0x1000) :
    {
        *(.text)
        *(.text.*)
    }

    /*
     * Constants and read-only data, such as string literals
     * that the compiler chooses not to place directly in code.
     */
    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /*
     * Global and static variables with explicit initial values.
     */
    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)
    }

    /*
     * Zero-initialized and uninitialized global storage.
     * This also contains the stack reserved by boot.asm.
     */
    .bss ALIGN(0x1000) :
    {
        bss_start = .;

        *(COMMON)
        *(.bss)
        *(.bss.*)

        bss_end = .;
    }

    kernel_end = .;

    /*
     * Metadata useful to development tools but unnecessary at runtime.
     */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.eh_frame.*)
    }
}
