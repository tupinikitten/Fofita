#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

void retro_input_poll_cb(void);
int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id);

#endif