#ifndef FOFITA_INPUT_RULES_H
#define FOFITA_INPUT_RULES_H

#include <stdbool.h>
#include <SDL2/SDL.h>

/* D-pad virtual gerado pelo stick esquerdo, com deadzone contra drift.
   Todas as direcoes em jogos continuam disponiveis pelo D-pad fisico. */
#define FOFITA_STICK_DEADZONE 16000
bool input_stick_direction(Sint16 axis, bool positive);

/* B e um botao do JOGO, nao atalho do dashboard. Sair usando Back+Start.
   O teclado preserva ESC/Espaco como atalhos de retorno. */
bool input_gamepad_exit_combo(bool in_game,
                              SDL_GameControllerButton just_pressed,
                              bool start_held, bool back_held);

#endif
