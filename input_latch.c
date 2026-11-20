#include "input_latch.h"
#include <string.h>

/* Todas as funcoes devem ser chamadas na thread principal SDL. Os pulsos
   sao consumidos uma vez por retro_input_poll, nao por botao consultado. */
static Uint8 key_pending[SDL_NUM_SCANCODES];
static Uint8 key_frame[SDL_NUM_SCANCODES];
static Uint8 button_pending[SDL_CONTROLLER_BUTTON_MAX];
static Uint8 button_frame[SDL_CONTROLLER_BUTTON_MAX];
static SDL_JoystickID button_pending_instance[SDL_CONTROLLER_BUTTON_MAX];
static SDL_JoystickID button_frame_instance[SDL_CONTROLLER_BUTTON_MAX];

void input_latch_reset(void) {
    memset(key_pending,0,sizeof(key_pending));
    memset(key_frame,0,sizeof(key_frame));
    memset(button_pending,0,sizeof(button_pending));
    memset(button_frame,0,sizeof(button_frame));
    memset(button_pending_instance,0,sizeof(button_pending_instance));
    memset(button_frame_instance,0,sizeof(button_frame_instance));
}

void input_latch_event(const SDL_Event *event) {
    if(!event) return;
    if(event->type==SDL_KEYDOWN && !event->key.repeat) {
        SDL_Scancode key=event->key.keysym.scancode;
        if(key>SDL_SCANCODE_UNKNOWN && key<SDL_NUM_SCANCODES)
            key_pending[key]=1;
    } else if(event->type==SDL_CONTROLLERBUTTONDOWN) {
        unsigned b=event->cbutton.button;
        if(b<SDL_CONTROLLER_BUTTON_MAX) {
            button_pending[b]=1;
            button_pending_instance[b]=event->cbutton.which;
        }
    } else if(event->type==SDL_WINDOWEVENT &&
              event->window.event==SDL_WINDOWEVENT_FOCUS_LOST) {
        input_latch_reset();
    } else if(event->type==SDL_CONTROLLERDEVICEREMOVED) {
        for(int i=0;i<SDL_CONTROLLER_BUTTON_MAX;++i) {
            if(button_pending_instance[i]==event->cdevice.which)
                button_pending[i]=0;
            if(button_frame_instance[i]==event->cdevice.which)
                button_frame[i]=0;
        }
    }
}

void input_latch_begin_frame(void) {
    memcpy(key_frame,key_pending,sizeof(key_frame));
    memset(key_pending,0,sizeof(key_pending));
    memcpy(button_frame,button_pending,sizeof(button_frame));
    memcpy(button_frame_instance,button_pending_instance,sizeof(button_frame_instance));
    memset(button_pending,0,sizeof(button_pending));
}

bool input_latch_key(SDL_Scancode key) {
    return key>SDL_SCANCODE_UNKNOWN && key<SDL_NUM_SCANCODES &&
           key_frame[key]!=0;
}

bool input_latch_button(SDL_JoystickID instance, SDL_GameControllerButton button) {
    return button>=SDL_CONTROLLER_BUTTON_A &&
           button<SDL_CONTROLLER_BUTTON_MAX &&
           button_frame[button]!=0 &&
           button_frame_instance[button]==instance;
}
