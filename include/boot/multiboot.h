#ifndef BOOT_MULTIBOOT_H
#define BOOT_MULTIBOOT_H

#include <stdint.h>

/* Value placed in EAX by a Multiboot-compliant bootloader. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u

/* flags bit 0: mem_lower and mem_upper contain valid values */
#define MULTIBOOT_INFO_MEMORY 0x00000001u

/*
 * Initial portion of the Multiboot information structure.
 *
 * Additional fields follow in memory, but the kernel does not interpret them
 * yet.
 */
struct multiboot_info_prefix {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
};

_Static_assert(sizeof(struct multiboot_info_prefix) == 12u,
               "Unexpected Multiboot information prefix size");

#endif
