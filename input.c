#include "input.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "libretro.h"
#include "input_rules.h"
#include "input_bindings.h"

extern bool em_menu; 

void retro_input_poll_cb(void) { 
    SDL_PumpEvents(); 
}

int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    if(em_menu || port!=0 || device!=RETRO_DEVICE_JOYPAD || index!=0)
        return 0;

    /* O mesmo mapa e usado para qualquer core: NES, GB/GBC e futuros.
       O remapeamento nao interfere com foco/atalhos do dashboard. */
    static const unsigned ids[INPUT_BIND_COUNT]={
        RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN,
        RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT,
        RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B,
        RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_SELECT
    };
    int action=-1;
    for(int a=0;a<INPUT_BIND_COUNT;++a)
        if(id==ids[a]) { action=a; break; }
    if(action<0) return 0;

    const Uint8 *keyboard=SDL_GetKeyboardState(NULL);
    SDL_Scancode scancode=input_bindings_keyboard((InputBindAction)action);
    if(keyboard && scancode>SDL_SCANCODE_UNKNOWN && scancode<SDL_NUM_SCANCODES &&
       keyboard[scancode]) return 1;

    SDL_GameController *pad=NULL;
    for(int j=0;j<SDL_NumJoysticks();++j) {
        if(SDL_IsGameController(j)) {
            pad=SDL_GameControllerFromInstanceID(SDL_JoystickGetDeviceInstanceID(j));
            if(pad) break;
        }
    }
    if(!pad) return 0;
    SDL_GameControllerButton button=input_bindings_gamepad((InputBindAction)action);
    if(button!=SDL_CONTROLLER_BUTTON_INVALID && SDL_GameControllerGetButton(pad,button))
        return 1;

    /* Analógico esquerdo permanece um segundo direcional, independente
       do botao fisico escolhido para UP/DOWN/LEFT/RIGHT. */
    Sint16 ax=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX);
    Sint16 ay=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);
    switch(action) {
    case INPUT_BIND_UP: return input_stick_direction(ay,false);
    case INPUT_BIND_DOWN: return input_stick_direction(ay,true);
    case INPUT_BIND_LEFT: return input_stick_direction(ax,false);
    case INPUT_BIND_RIGHT: return input_stick_direction(ax,true);
    default: return 0;
    }
}
