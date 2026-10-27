#include "input.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "libretro.h"

extern bool em_menu; 

void retro_input_poll_cb(void) { 
    SDL_PumpEvents(); 
}

int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) { 
    if (em_menu) return 0;
    
    if (port != 0 || device != RETRO_DEVICE_JOYPAD || index != 0) return 0;
    
    const Uint8 *teclas = SDL_GetKeyboardState(NULL);
    switch (id) {
        case RETRO_DEVICE_ID_JOYPAD_UP:     return teclas[SDL_SCANCODE_UP];
        case RETRO_DEVICE_ID_JOYPAD_DOWN:   return teclas[SDL_SCANCODE_DOWN];
        case RETRO_DEVICE_ID_JOYPAD_LEFT:   return teclas[SDL_SCANCODE_LEFT];
        case RETRO_DEVICE_ID_JOYPAD_RIGHT:  return teclas[SDL_SCANCODE_RIGHT];
        case RETRO_DEVICE_ID_JOYPAD_START:  return teclas[SDL_SCANCODE_RETURN];
        case RETRO_DEVICE_ID_JOYPAD_SELECT: return teclas[SDL_SCANCODE_RSHIFT];
        case RETRO_DEVICE_ID_JOYPAD_A:      return teclas[SDL_SCANCODE_X];
        case RETRO_DEVICE_ID_JOYPAD_B:      return teclas[SDL_SCANCODE_Z];
        default: return 0;
    }
}