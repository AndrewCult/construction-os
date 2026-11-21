#include "arch/i386/idt.h"
#include "arch/i386/io.h"
#include "arch/i386/pic.h"
#include "arch/i386/pit.h"
#include "boot/multiboot.h"
#include "kernel/serial.h"
#include "kernel/terminal.h"
#include <stdbool.h>
#include <stdint.h>

#define PS2_DATA_PORT 0x60u
#define KEYBOARD_QUEUE_SIZE 4u

enum key_code {
  KEY_RIGHT_ARROW,
  KEY_NUM_LOCK,
  KEY_LEFT_ARROW,
  KEY_UP_ARROW,
  KEY_DOWN_ARROW
};

struct key_event {
  enum key_code key;
  bool pressed;
};

/*
 * Write a 32-bit unsigned integer in decimal form to both diagnostic outputs.
 */
static void write_uint32_decimal(uint32_t value) {
  char digits[11];
  unsigned int length = 0u;

  /*
   * Extract digits from right to left.
   *
   * The do-while is necessary because the value zero must still produce one
   * digit.
   */
  do {
    digits[length] = (char)('0' + (value % 10u));
    ++length;
    value /= 10u;
  } while (value != 0u);

  /*
   * The digits were generated in reverse order, so reverse the buffer.
   */
  for (unsigned int left = 0u, right = length - 1u; left < right;
       ++left, --right) {
    const char temporary = digits[left];
    digits[left] = digits[right];
    digits[right] = temporary;
  }

  digits[length] = '\0';

  terminal_write(digits);
  serial_write(digits);
}

static volatile uint32_t timer_ticks;
static uint8_t keyboard_queue[KEYBOARD_QUEUE_SIZE];
static volatile unsigned int keyboard_write_index;
static volatile unsigned int keyboard_read_index;
static volatile uint32_t keyboard_dropped_bytes;

void irq0_handler(void) {
  ++timer_ticks;
  pic_send_eoi(0u);
}

static void print_keyboard_byte(uint8_t byte) {
  static const char hex_digits[] = "0123456789ABCDEF";
  char value[] = "0x00\n";

  value[2] = hex_digits[byte >> 4];
  value[3] = hex_digits[byte & 0x0Fu];

  serial_write("IRQ1: ");
  serial_write(value);
}

static bool decode_keyboard_byte(uint8_t byte, struct key_event *event) {
  static bool extended_pending = false;

  if (byte == 0xE0u) {
    extended_pending = true;
    return false;
  }

  if (!extended_pending) {
    if (byte == 0x45u || byte == 0xC5u) {
      event->key = KEY_NUM_LOCK;
      event->pressed = (byte == 0x45u);
      return true;
    }

    return false;
  }

  extended_pending = false;

  const uint8_t code = byte & 0x7Fu;

  switch (code) {
  case 0x4Du:
    event->key = KEY_RIGHT_ARROW;
    break;
  case 0x4Bu:
    event->key = KEY_LEFT_ARROW;
    break;
  case 0x48u:
    event->key = KEY_UP_ARROW;
    break;
  case 0x50u:
    event->key = KEY_DOWN_ARROW;
    break;
  default:
    return false;
  }

  event->pressed = (byte & 0x80u) == 0u;
  return true;
}

void irq1_handler(void) {
  const uint8_t byte = inb(PS2_DATA_PORT);
  const unsigned int next = (keyboard_write_index + 1u) % KEYBOARD_QUEUE_SIZE;

  if (next != keyboard_read_index) {
    keyboard_queue[keyboard_write_index] = byte;
    keyboard_write_index = next;
  } else {
    ++keyboard_dropped_bytes;
  }

  pic_send_eoi(1u);
}

void kernel_main(unsigned int multiboot_magic,
                 unsigned int multiboot_info_address) {
  terminal_initialize();
  serial_initialize();

  if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    terminal_write("ERROR: Invalid Multiboot magic value.\n");
    serial_write("ERROR: Invalid Multiboot magic value.\n");

    for (;;) {
      __asm__ volatile("cli; hlt");
    }
  }

  /*
   * The Multiboot information structure is not inspected yet.
   * Explicitly mark its address as intentionally unused.
   */
  const struct multiboot_info_prefix *multiboot_info =
      (const struct multiboot_info_prefix *)(uintptr_t)multiboot_info_address;

  if ((multiboot_info->flags & MULTIBOOT_INFO_MEMORY) != 0u) {
    terminal_write("Multiboot basic memory information is available.\n");
    serial_write("Multiboot basic memory information is available.\n");

    terminal_write("Lower memory: ");
    serial_write("Lower memory: ");
    write_uint32_decimal(multiboot_info->mem_lower);
    terminal_write(" KiB\n");
    serial_write(" KiB\n");

    terminal_write("Upper memory: ");
    serial_write("Upper memory: ");
    write_uint32_decimal(multiboot_info->mem_upper);
    terminal_write(" KiB\n");
    serial_write(" KiB\n");
  } else {
    terminal_write("Multiboot basic memory information is unavailable.\n");
    serial_write("Multiboot basic memory information is unavailable.\n");
  }

  idt_initialize();
  pic_initialize();

  pit_initialize();
  serial_write("PIT: waiting for IRQ0 ticks\n");
  pic_unmask_irq(0u);

  while (timer_ticks < 2u) {
    __asm__ volatile("sti\n\thlt\n\tcli" ::: "memory");
  }

  __asm__ volatile("sti" ::: "memory");
  serial_write("PIT: received at least two IRQ0 ticks\n");

  terminal_write("Hello from construction-os!\n");
  terminal_write("The kernel is running in 32-bit protected mode.\n");
  terminal_write("Multiboot information received successfully.\n");

  serial_write("Hello from construction-os!\n");
  serial_write("The kernel is running in 32-bit protected mode.\n");
  serial_write("Multiboot information received successfully.\n");

  const uint32_t ticks_at_boot = timer_ticks;
  while (timer_ticks - ticks_at_boot < 100u) {
    __asm__ volatile("hlt" ::: "memory");
  }
  serial_write("PIT: 100 ticks after boot\n");
  pic_unmask_irq(1u);

  uint32_t reported_drops = 0u;

  for (;;) {
    __asm__ volatile("cli" ::: "memory");

    if (keyboard_read_index == keyboard_write_index) {
      __asm__ volatile("sti\n\thlt" ::: "memory");
      continue;
    }

    const uint8_t byte = keyboard_queue[keyboard_read_index];
    keyboard_read_index = (keyboard_read_index + 1u) % KEYBOARD_QUEUE_SIZE;
    const uint32_t drops = keyboard_dropped_bytes;

    __asm__ volatile("sti" ::: "memory");
    print_keyboard_byte(byte);

    struct key_event event;
    if (decode_keyboard_byte(byte, &event)) {
      switch (event.key) {
      case KEY_RIGHT_ARROW:
        serial_write("Keyboard: right arrow");
        break;
      case KEY_NUM_LOCK:
        serial_write("Keyboard: num lock");
        break;
      case KEY_LEFT_ARROW:
        serial_write("Keyboard: left arrow");
        break;
      case KEY_UP_ARROW:
        serial_write("Keyboard: up arrow");
        break;
      case KEY_DOWN_ARROW:
        serial_write("Keyboard: down arrow");
        break;
      }
      serial_write(event.pressed ? " pressed\n" : " released\n");
    }

    if (drops != reported_drops) {
      const char *message = "Keyboard bytes dropped: ";
      terminal_write(message);
      serial_write(message);
      write_uint32_decimal(drops);
      terminal_write("\n");
      serial_write("\n");
      reported_drops = drops;
    }
  }
}
