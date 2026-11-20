#include <assert.h>
#include <stdio.h>
#include "../input_rules.h"

int main(void) {
    assert(!input_stick_direction(0,true));
    assert(!input_stick_direction(0,false));
    assert(!input_stick_direction(15999,true));
    assert(!input_stick_direction(-15999,false));
    assert(input_stick_direction(16000,true));
    assert(input_stick_direction(-16000,false));
    assert(!input_stick_direction(-20000,true));
    assert(!input_stick_direction(20000,false));

    /* B nunca fecha o jogo, mesmo quando os modificadores estao ativos. */
    assert(!input_gamepad_exit_combo(true,SDL_CONTROLLER_BUTTON_B,false,false));
    assert(!input_gamepad_exit_combo(true,SDL_CONTROLLER_BUTTON_B,true,true));
    assert(!input_gamepad_exit_combo(true,SDL_CONTROLLER_BUTTON_START,true,false));
    assert(!input_gamepad_exit_combo(true,SDL_CONTROLLER_BUTTON_BACK,false,true));
    assert(input_gamepad_exit_combo(true,SDL_CONTROLLER_BUTTON_START,true,true));
    assert(input_gamepad_exit_combo(true,SDL_CONTROLLER_BUTTON_BACK,true,true));
    assert(!input_gamepad_exit_combo(false,SDL_CONTROLLER_BUTTON_START,true,true));

    puts("B2.1: gamepad (B, Back+Start, analogico) OK");
    return 0;
}
