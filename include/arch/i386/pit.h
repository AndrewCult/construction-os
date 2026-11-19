#ifndef ARCH_I386_PIT_H
#define ARCH_I386_PIT_H

/*
 * Configure PIT channel 0 to generate approximately 100 ticks per second.
 */
void pit_initialize(void);

#endif
