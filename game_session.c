#include "game_session.h"

static GameSessionState estado = GAME_SESSION_MENU;
static GameSessionError erro = GAME_SESSION_OK;
static CoreCapabilities recursos;
/* Somente uma sessao que inicializou o core com sucesso e dona dele. */
static bool core_ativo = false;
static unsigned state_slot=1;

/* Estados e capacidades sao exclusivos da sessao atualmente aberta.
   Nao sao gravados em configuracao, nem atualizados pela interface. */
GameSessionState game_session_state(void) { return estado; }
GameSessionError game_session_last_error(void) { return erro; }
const CoreCapabilities *game_session_capabilities(void) { return &recursos; }
unsigned game_session_state_slot(void) { return state_slot; }
unsigned game_session_next_state_slot(void) {
    if(estado!=GAME_SESSION_PLAYING && estado!=GAME_SESSION_PAUSED) return state_slot;
    state_slot=state_slot%FOFITA_STATE_SLOTS+1;
    return state_slot;
}
StateIOResult game_session_save_state(void) {
    if(estado!=GAME_SESSION_PLAYING && estado!=GAME_SESSION_PAUSED)
        return STATE_IO_UNSUPPORTED;
    return core_save_state(state_slot);
}
StateIOResult game_session_load_state(void) {
    if(estado!=GAME_SESSION_PLAYING && estado!=GAME_SESSION_PAUSED)
        return STATE_IO_UNSUPPORTED;
    return core_load_state(state_slot);
}

bool game_session_start(const char *core_path, const char *game_path) {
    if(estado==GAME_SESSION_STARTING || estado==GAME_SESSION_PLAYING ||
       estado==GAME_SESSION_PAUSED || estado==GAME_SESSION_STOPPING) {
        erro=GAME_SESSION_ERROR_BUSY;
        return false;
    }
    recursos=(CoreCapabilities){0};
    if(!core_path || !core_path[0] || !game_path || !game_path[0]) {
        erro=GAME_SESSION_ERROR_ARGUMENT;
        estado=GAME_SESSION_ERROR;
        return false;
    }

    estado=GAME_SESSION_STARTING;
    erro=GAME_SESSION_OK;
    state_slot=1;

    if(!init_core(core_path)) {
        close_core(); /* Libera qualquer inicializacao parcial do loader. */
        erro=GAME_SESSION_ERROR_CORE;
        estado=GAME_SESSION_ERROR;
        return false;
    }
    core_ativo=true;
    if(!load_game(game_path)) {
        close_core();
        core_ativo=false;
        erro=GAME_SESSION_ERROR_GAME;
        estado=GAME_SESSION_ERROR;
        return false;
    }

    const CoreCapabilities *encontrados=core_get_capabilities();
    if(encontrados) recursos=*encontrados;
    estado=GAME_SESSION_PLAYING;
    return true;
}

bool game_session_set_paused(bool paused) {
    if(paused && estado==GAME_SESSION_PLAYING) {
        estado=GAME_SESSION_PAUSED;
        return true;
    }
    if(!paused && estado==GAME_SESSION_PAUSED) {
        estado=GAME_SESSION_PLAYING;
        return true;
    }
    return (paused && estado==GAME_SESSION_PAUSED) ||
           (!paused && estado==GAME_SESSION_PLAYING);
}

void game_session_run_frame(void) {
    if(estado==GAME_SESSION_PLAYING) run_core();
}

void game_session_stop(void) {
    if(estado==GAME_SESSION_MENU) return;
    estado=GAME_SESSION_STOPPING;
    if(core_ativo) close_core();
    core_ativo=false;
    recursos=(CoreCapabilities){0};
    erro=GAME_SESSION_OK;
    estado=GAME_SESSION_MENU;
    state_slot=1;
}
