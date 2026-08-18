/* VGA text-mode dimensions. */
#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u

/* VGA text buffer mapped into the physical address space. */
#define VGA_MEMORY ((volatile unsigned short *)0xB8000)

void kernel_main(void) {
  const char message[] = "Hello from construction-os!";
  const unsigned char foreground = 15u;
  const unsigned char background = 0u;
  const unsigned char color = (unsigned char)(foreground | (background << 4));

  for (unsigned int column = 0u; message[column] != '\0'; ++column) {
    const unsigned short character = (unsigned char)message[column];
    const unsigned short attribute = (unsigned short)color << 8;

    VGA_MEMORY[column] = character | attribute;
  }
}
