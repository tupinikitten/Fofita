#ifndef FOFITA_INPUT_BINDINGS_H
#define FOFITA_INPUT_BINDINGS_H

#include <stdbool.h>
#include <SDL2/SDL.h>

/* A ordem e a mesma para teclado e controle. Exclusivo da jogabilidade:
   navegacao do dashboard e atalhos de emergencia nao sao remapeados. */
typedef enum {
    INPUT_BIND_UP = 0,
    INPUT_BIND_DOWN,
    INPUT_BIND_LEFT,
    INPUT_BIND_RIGHT,
    INPUT_BIND_A,
    INPUT_BIND_B,
    INPUT_BIND_START,
    INPUT_BIND_SELECT,
    INPUT_BIND_COUNT
} InputBindAction;

void input_bindings_reset(void);
void input_bindings_initialize(void);

SDL_Scancode input_bindings_keyboard(InputBindAction action);
SDL_GameControllerButton input_bindings_gamepad(InputBindAction action);

/* Se a tecla/botao escolhido ja estiver em uso, as duas acoes trocam
   de lugar, mantendo um mapa sem duplicatas. ESC e ESPACO sao reservados
   para sair da partida pelo teclado. */
bool input_bindings_set_keyboard(InputBindAction action, SDL_Scancode key);
bool input_bindings_set_gamepad(InputBindAction action, SDL_GameControllerButton button);

/* Persistencia em pasta do usuario. Funcoes _file permitem testes isolados. */
bool input_bindings_save(void);
bool input_bindings_save_file(const char *path);
bool input_bindings_load_file(const char *path);

#endif
