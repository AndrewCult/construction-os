#include "kernel/terminal.h"

/* Standard VGA text-mode geometry. */
#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u

/* Physical address at which the VGA text buffer is mapped. */
#define VGA_MEMORY ((volatile unsigned short *)0xB8000)

/*
 * VGA text-mode colors. Each value occupies four bits inside
 * the attribute byte of a screen cell.
 */
enum vga_color {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15
};

/* Current terminal state. */
static unsigned int terminal_row;
static unsigned int terminal_column;
static unsigned char terminal_color;

/*
 * Combine foreground and background colors into the VGA attribute byte.
 */
static unsigned char vga_make_color(enum vga_color foreground,
                                    enum vga_color background) {
  return (unsigned char)((unsigned char)foreground |
                         ((unsigned char)background << 4));
}

/*
 * Combine an ASCII character and a color attribute into one VGA cell.
 */
static unsigned short vga_make_entry(unsigned char character,
                                     unsigned char color) {
  return (unsigned short)character | ((unsigned short)color << 8);
}

/*
 * Return the linear VGA buffer index for a row and column.
 */
static unsigned int vga_index(unsigned int row, unsigned int column) {
  return row * VGA_WIDTH + column;
}

/*
 * Advance to the next row.
 *
 * Scrolling is not implemented yet. Reaching the bottom temporarily
 * wraps the cursor back to the first row.
 */
static void terminal_advance_line(void) {
  terminal_column = 0u;
  ++terminal_row;

  if (terminal_row == VGA_HEIGHT) {
    terminal_row = 0u;
  }
}

/*
 * Write one character and update the software cursor.
 */
static void terminal_putchar(char character) {
  if (character == '\n') {
    terminal_advance_line();
    return;
  }

  VGA_MEMORY[vga_index(terminal_row, terminal_column)] =
      vga_make_entry((unsigned char)character, terminal_color);

  ++terminal_column;

  if (terminal_column == VGA_WIDTH) {
    terminal_advance_line();
  }
}

void terminal_initialize(void) {
  terminal_row = 0u;
  terminal_column = 0u;
  terminal_color = vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

  const unsigned short blank =
      vga_make_entry((unsigned char)' ', terminal_color);

  for (unsigned int row = 0u; row < VGA_HEIGHT; ++row) {
    for (unsigned int column = 0u; column < VGA_WIDTH; ++column) {
      VGA_MEMORY[vga_index(row, column)] = blank;
    }
  }
}

void terminal_write(const char *text) {
  for (unsigned int index = 0u; text[index] != '\0'; ++index) {
    terminal_putchar(text[index]);
  }
}
