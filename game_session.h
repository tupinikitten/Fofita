#ifndef FOFITA_GAME_SESSION_H
#define FOFITA_GAME_SESSION_H

#include <stdbool.h>
#include "libretro_core.h"

/* Estado do backend: independente da sidebar, carrossel e renderer. */
typedef enum {
    GAME_SESSION_MENU = 0,
    GAME_SESSION_STARTING,
    GAME_SESSION_PLAYING,
    GAME_SESSION_PAUSED,
    GAME_SESSION_STOPPING,
    GAME_SESSION_ERROR
} GameSessionState;

typedef enum {
    GAME_SESSION_OK = 0,
    GAME_SESSION_ERROR_BUSY,
    GAME_SESSION_ERROR_ARGUMENT,
    GAME_SESSION_ERROR_CORE,
    GAME_SESSION_ERROR_GAME
} GameSessionError;

/* Usar na thread principal. O menu rapido consumira esta interface
   quando for implementado, sem acessar diretamente o Libretro. */
GameSessionState game_session_state(void);
GameSessionError game_session_last_error(void);
const CoreCapabilities *game_session_capabilities(void);

bool game_session_start(const char *core_path, const char *game_path);
bool game_session_set_paused(bool paused);
void game_session_run_frame(void);
void game_session_stop(void);

/* Tres slots por jogo. A selecao reinicia em 1 a cada novo jogo. */
unsigned game_session_state_slot(void);
unsigned game_session_next_state_slot(void);
StateIOResult game_session_save_state(void);
StateIOResult game_session_load_state(void);

#endif
