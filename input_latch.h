#ifndef FOFITA_INPUT_LATCH_H
#define FOFITA_INPUT_LATCH_H

#include <stdbool.h>
#include <SDL2/SDL.h>

/* Guarda pulsos SDL que duraram menos de um frame Libretro. A leitura
   instantanea via SDL_GetKeyboardState pode perder esses eventos. */
void input_latch_reset(void);
void input_latch_event(const SDL_Event *event);
void input_latch_begin_frame(void);
bool input_latch_key(SDL_Scancode key);
bool input_latch_button(SDL_JoystickID instance, SDL_GameControllerButton button);

#endif
