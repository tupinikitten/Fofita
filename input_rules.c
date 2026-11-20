#include "input_rules.h"

bool input_stick_direction(Sint16 axis, bool positive) {
    return positive ? axis >= FOFITA_STICK_DEADZONE
                    : axis <= -FOFITA_STICK_DEADZONE;
}

bool input_gamepad_exit_combo(bool in_game,
                              SDL_GameControllerButton just_pressed,
                              bool start_held, bool back_held) {
    if(!in_game || !start_held || !back_held) return false;
    return just_pressed == SDL_CONTROLLER_BUTTON_START ||
           just_pressed == SDL_CONTROLLER_BUTTON_BACK;
}
