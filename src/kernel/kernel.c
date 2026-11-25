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
  KEY_DOWN_ARROW,
  /* Keep letters contiguous so their key codes can be used as alphabet offsets.
   */
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_LEFT_SHIFT,
  KEY_RIGHT_SHIFT,
  KEY_CAPS_LOCK,
  KEY_SPACE,
  KEY_ENTER,
  KEY_BACKSPACE,
  /* Top-row digits in scan-code order. */
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,
  KEY_COMMA,
  KEY_PERIOD,
  KEY_MINUS,
  KEY_APOSTROPHE,
  KEY_PLUS,
  KEY_BACKSLASH,
  KEY_LESS_THAN,
  KEY_I_GRAVE,
  KEY_E_GRAVE,
  KEY_O_GRAVE,
  KEY_A_GRAVE,
  KEY_U_GRAVE,
  KEY_ALT_GR,
  KEY_KP_0,
  KEY_KP_1,
  KEY_KP_2,
  KEY_KP_3,
  KEY_KP_4,
  KEY_KP_5,
  KEY_KP_6,
  KEY_KP_7,
  KEY_KP_8,
  KEY_KP_9,
  KEY_KP_MULTIPLY,
  KEY_KP_SUBTRACT,
  KEY_KP_ADD,
  KEY_KP_DECIMAL,
  KEY_KP_DIVIDE,
  KEY_KP_ENTER
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

/* Keyboard input queue.
 *
 * IRQ1 advances the write index. The main loop advances the read index.
 * One slot stays empty so equal indices always mean an empty queue.
 */
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

/* ==== Keyboard decoding ==== */

static const char letter_by_scancode[128] = {
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x2C] = 'z',
    [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n',
    [0x32] = 'm',
};

static const char keypad_digit_by_scancode[128] = {
    [0x47] = '7', [0x48] = '8', [0x49] = '9', [0x4B] = '4', [0x4C] = '5',
    [0x4D] = '6', [0x4F] = '1', [0x50] = '2', [0x51] = '3', [0x52] = '0',
};

static const char top_row_digits[] = "1234567890";

/* In VGA code page 437, the pound sign occupies slot 0x9C. */
static const char italian_shifted_digits[] = {'!', '"', '\x9C', '$', '%',
                                              '&', '/', '(',    ')', '='};

/* Return true when a byte completes a recognized key event.
 * An E0 prefix is remembered for the next call. Unknown codes return false.
 */
static bool decode_keyboard_byte(uint8_t byte, struct key_event *event) {
  static bool extended_pending = false;

  if (byte == 0xE0u) {
    extended_pending = true;
    return false;
  }

  const uint8_t code = byte & 0x7Fu;

  if (!extended_pending) {
    switch (code) {
    case 0x45u:
      event->key = KEY_NUM_LOCK;
      break;
    case 0x2Au:
      event->key = KEY_LEFT_SHIFT;
      break;
    case 0x36u:
      event->key = KEY_RIGHT_SHIFT;
      break;
    case 0x3Au:
      event->key = KEY_CAPS_LOCK;
      break;
    case 0x39u:
      event->key = KEY_SPACE;
      break;
    case 0x1Cu:
      event->key = KEY_ENTER;
      break;
    case 0x0Eu:
      event->key = KEY_BACKSPACE;
      break;
    case 0x33u:
      event->key = KEY_COMMA;
      break;
    case 0x34u:
      event->key = KEY_PERIOD;
      break;
    case 0x35u:
      event->key = KEY_MINUS;
      break;
    case 0x0C:
      event->key = KEY_APOSTROPHE;
      break;
    case 0x1B:
      event->key = KEY_PLUS;
      break;
    case 0x29:
      event->key = KEY_BACKSLASH;
      break;
    case 0x56:
      event->key = KEY_LESS_THAN;
      break;
    case 0x0D:
      event->key = KEY_I_GRAVE;
      break;
    case 0x1A:
      event->key = KEY_E_GRAVE;
      break;
    case 0x27:
      event->key = KEY_O_GRAVE;
      break;
    case 0x28:
      event->key = KEY_A_GRAVE;
      break;
    case 0x2B:
      event->key = KEY_U_GRAVE;
      break;
    case 0x37:
      event->key = KEY_KP_MULTIPLY;
      break;
    case 0x4A:
      event->key = KEY_KP_SUBTRACT;
      break;
    case 0x4E:
      event->key = KEY_KP_ADD;
      break;
    case 0x53:
      event->key = KEY_KP_DECIMAL;
      break;
    default: {
      if (code >= 0x02u && code <= 0x0Bu) {
        event->key = (enum key_code)(KEY_1 + (code - 0x02u));
        break;
      }
      char digit = keypad_digit_by_scancode[code];
      if (digit != '\0') {
        event->key = (enum key_code)(KEY_KP_0 + (digit - '0'));
        break;
      }
      char letter = letter_by_scancode[code];
      if (letter == '\0') {
        return false;
      }
      event->key = (enum key_code)(KEY_A + (letter - 'a'));
      break;
    }
    }

    event->pressed = (byte & 0x80u) == 0u;
    return true;
  }

  extended_pending = false;

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
  case 0x38u:
    event->key = KEY_ALT_GR;
    break;
  case 0x35u:
    event->key = KEY_KP_DIVIDE;
    break;
  case 0x1Cu:
    event->key = KEY_KP_ENTER;
    break;
  default:
    return false;
  }

  event->pressed = (byte & 0x80u) == 0u;
  return true;
}

static bool key_event_to_char(const struct key_event *event, bool shift_down,
                              bool caps_lock_on, bool altgr_down,
                              bool num_lock_on, char *character) {
  if (!event->pressed) {
    return false;
  }

  if (event->key == KEY_SPACE) {
    *character = ' ';
    return true;
  }

  if (event->key == KEY_ENTER || event->key == KEY_KP_ENTER) {
    *character = '\n';
    return true;
  }

  if (event->key == KEY_BACKSPACE) {
    *character = '\b';
    return true;
  }

  if (altgr_down) {
    switch (event->key) {
    case KEY_E_GRAVE:
      *character = shift_down ? '{' : '[';
      return true;
    case KEY_PLUS:
      *character = shift_down ? '}' : ']';
      return true;
    case KEY_O_GRAVE:
      if (shift_down) {
        return false;
      }
      *character = '@';
      return true;
    case KEY_A_GRAVE:
      if (shift_down) {
        return false;
      }
      *character = '#';
      return true;
    default:
      return false;
    }
  }

  if (event->key >= KEY_KP_0 && event->key <= KEY_KP_9) {
    if (!num_lock_on) {
      return false;
    }

    *character = (char)('0' + (event->key - KEY_KP_0));
    return true;
  }

  if (event->key >= KEY_1 && event->key <= KEY_0) {
    unsigned int index = (unsigned int)(event->key - KEY_1);
    *character =
        shift_down ? italian_shifted_digits[index] : top_row_digits[index];
    return true;
  }

  if (event->key >= KEY_A && event->key <= KEY_Z) {
    char first = (shift_down != caps_lock_on) ? 'A' : 'a';
    *character = (char)(first + (event->key - KEY_A));
    return true;
  }

  switch (event->key) {
  case KEY_COMMA:
    *character = shift_down ? ';' : ',';
    return true;
  case KEY_PERIOD:
    *character = shift_down ? ':' : '.';
    return true;
  case KEY_MINUS:
    *character = shift_down ? '_' : '-';
    return true;
  case KEY_APOSTROPHE:
    *character = shift_down ? '?' : '\'';
    return true;
  case KEY_PLUS:
    *character = shift_down ? '*' : '+';
    return true;
  case KEY_BACKSLASH:
    *character = shift_down ? '|' : '\\';
    return true;
  case KEY_LESS_THAN:
    *character = shift_down ? '>' : '<';
    return true;
  case KEY_I_GRAVE:
    *character = shift_down ? '^' : '\x8D'; /* ì / ^ */
    return true;
  case KEY_E_GRAVE:
    *character = shift_down ? '\x82' : '\x8A'; /* è / é */
    return true;
  case KEY_O_GRAVE:
    *character = shift_down ? '\x87' : '\x95'; /* ò / ç */
    return true;
  case KEY_A_GRAVE:
    *character = shift_down ? '\xF8' : '\x85'; /* à / ° */
    return true;
  case KEY_U_GRAVE:
    *character = shift_down ? '\x15' : '\x97'; /* ù / § */
    return true;
  case KEY_KP_MULTIPLY:
    *character = '*';
    return true;
  case KEY_KP_SUBTRACT:
    *character = '-';
    return true;
  case KEY_KP_ADD:
    *character = '+';
    return true;
  case KEY_KP_DIVIDE:
    *character = '/';
    return true;
  case KEY_KP_DECIMAL:
    if (!num_lock_on) {
      return false;
    }
    *character = '.';
    return true;
  default:
    return false;
  }
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

  /* Report basic memory information when the bootloader provides it. */
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

  /* ===== Keyboard event loop. ===== */
  uint32_t reported_drops = 0u;
  bool left_shift_down = false;
  bool right_shift_down = false;
  bool caps_lock_down = false;
  bool caps_lock_on = false;
  bool altgr_down = false;
  bool num_lock_down = false;
  bool num_lock_on = false;

  for (;;) {
    __asm__ volatile("cli" ::: "memory");

    if (keyboard_read_index == keyboard_write_index) {
      /* Keep STI and HLT together so an IRQ arriving after the empty
       * check wakes the CPU instead of being handled just before sleep.
       */
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
      if (event.key == KEY_LEFT_SHIFT) {
        left_shift_down = event.pressed;
      } else if (event.key == KEY_RIGHT_SHIFT) {
        right_shift_down = event.pressed;
      } else if (event.key == KEY_CAPS_LOCK) {
        /* Toggle once per physical press, even if a press event repeats. */
        if (event.pressed && !caps_lock_down) {
          caps_lock_on = !caps_lock_on;
        }
        caps_lock_down = event.pressed;
      } else if (event.key == KEY_ALT_GR) {
        altgr_down = event.pressed;
      } else if (event.key == KEY_NUM_LOCK) {
        if (event.pressed && !num_lock_down) {
          num_lock_on = !num_lock_on;
        }
        num_lock_down = event.pressed;
      }

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
      case KEY_LEFT_SHIFT:
        serial_write("Keyboard: left shift");
        break;
      case KEY_RIGHT_SHIFT:
        serial_write("Keyboard: right shift");
        break;
      case KEY_CAPS_LOCK:
        serial_write("Keyboard: caps lock");
        break;
      case KEY_SPACE:
        serial_write("Keyboard: space");
        break;
      case KEY_ENTER:
        serial_write("Keyboard: enter");
        break;
      case KEY_BACKSPACE:
        serial_write("Keyboard: backspace");
        break;
      case KEY_COMMA:
        serial_write("Keyboard: comma");
        break;
      case KEY_PERIOD:
        serial_write("Keyboard: period");
        break;
      case KEY_MINUS:
        serial_write("Keyboard: minus");
        break;
      case KEY_APOSTROPHE:
        serial_write("Keyboard: apostrophe");
        break;
      case KEY_PLUS:
        serial_write("Keyboard: plus");
        break;
      case KEY_BACKSLASH:
        serial_write("Keyboard: backslash");
        break;
      case KEY_LESS_THAN:
        serial_write("Keyboard: less than");
        break;
      case KEY_I_GRAVE:
        serial_write("Keyboard: i grave");
        break;
      case KEY_E_GRAVE:
        serial_write("Keyboard: e grave");
        break;
      case KEY_O_GRAVE:
        serial_write("Keyboard: o grave");
        break;
      case KEY_A_GRAVE:
        serial_write("Keyboard: a grave");
        break;
      case KEY_U_GRAVE:
        serial_write("Keyboard: u grave");
        break;
      case KEY_ALT_GR:
        serial_write("Keyboard: altgr");
        break;
      default:
        if (event.key >= KEY_A && event.key <= KEY_Z) {
          char name[] = {(char)('A' + (event.key - KEY_A)), '\0'};
          serial_write("Keyboard: ");
          serial_write(name);
        } else if (event.key >= KEY_1 && event.key <= KEY_0) {
          char name[] = {top_row_digits[event.key - KEY_1], '\0'};
          serial_write("Keyboard: ");
          serial_write(name);
        } else if (event.key >= KEY_KP_0 && event.key <= KEY_KP_9) {
          char name[] = {(char)('0' + (event.key - KEY_KP_0)), '\0'};
          serial_write("Keyboard: keypad ");
          serial_write(name);
        } else {
          serial_write("Keyboard: unknown key");
        }
        break;
      }
      serial_write(event.pressed ? " pressed\n" : " released\n");

      char character;
      if (key_event_to_char(&event, left_shift_down || right_shift_down,
                            caps_lock_on, altgr_down, num_lock_on,
                            &character)) {
        terminal_putchar(character);
      }
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
