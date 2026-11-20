#include <assert.h>
#include <stdio.h>
#include "../game_session.h"

/* Simula o Libretro; testes rodam sem DLL, SDL ou ROM. */
static int permitir_core=1, permitir_rom=1;
static int inicializacoes=0, cargas=0, quadros=0, fechamentos=0;
static CoreCapabilities core_recursos;
static unsigned last_state_slot=0;
static int save_state_calls=0,load_state_calls=0;

bool init_core(const char *path) {
    assert(path && path[0]);
    inicializacoes++;
    return permitir_core!=0;
}
bool load_game(const char *path) {
    assert(path && path[0]);
    cargas++;
    return permitir_rom!=0;
}
void run_core(void) { quadros++; }
void unload_game(void) {}
void close_core(void) { fechamentos++; }
const CoreCapabilities *core_get_capabilities(void) { return &core_recursos; }
StateIOResult core_save_state(unsigned slot) {
    last_state_slot=slot;
    ++save_state_calls;
    return STATE_IO_OK;
}
StateIOResult core_load_state(unsigned slot) {
    last_state_slot=slot;
    ++load_state_calls;
    return STATE_IO_EMPTY;
}

int main(void) {
    assert(game_session_state()==GAME_SESSION_MENU);
    assert(game_session_last_error()==GAME_SESSION_OK);
    assert(game_session_state_slot()==1);
    assert(game_session_save_state()==STATE_IO_UNSUPPORTED);
    assert(game_session_load_state()==STATE_IO_UNSUPPORTED);
    assert(game_session_next_state_slot()==1);
    game_session_run_frame();
    assert(quadros==0);

    assert(!game_session_start(NULL,"teste.nes"));
    assert(game_session_state()==GAME_SESSION_ERROR);
    assert(game_session_last_error()==GAME_SESSION_ERROR_ARGUMENT);
    assert(inicializacoes==0);
    game_session_stop();
    assert(game_session_state()==GAME_SESSION_MENU);

    permitir_core=0;
    assert(!game_session_start("core.dll","teste.nes"));
    assert(game_session_last_error()==GAME_SESSION_ERROR_CORE);
    assert(game_session_state()==GAME_SESSION_ERROR);
    assert(fechamentos==1);
    permitir_core=1;

    permitir_rom=0;
    assert(!game_session_start("core.dll","teste.nes"));
    assert(game_session_last_error()==GAME_SESSION_ERROR_GAME);
    assert(game_session_state()==GAME_SESSION_ERROR);
    assert(fechamentos==2);
    permitir_rom=1;

    core_recursos.save_states_available=true;
    core_recursos.save_state_bytes=4096;
    core_recursos.save_ram_available=true;
    core_recursos.save_ram_bytes=2048;
    core_recursos.cheat_api_present=true;

    assert(game_session_start("core.dll","teste.nes"));
    assert(game_session_state()==GAME_SESSION_PLAYING);
    assert(game_session_last_error()==GAME_SESSION_OK);
    assert(game_session_capabilities()->save_state_bytes==4096);
    assert(game_session_capabilities()->save_ram_bytes==2048);
    assert(game_session_capabilities()->cheat_api_present);
    assert(!game_session_start("outra.dll","outro.nes"));
    assert(game_session_last_error()==GAME_SESSION_ERROR_BUSY);
    assert(game_session_state()==GAME_SESSION_PLAYING);

    game_session_run_frame();
    assert(quadros==1);
    assert(game_session_state_slot()==1);
    assert(game_session_save_state()==STATE_IO_OK);
    assert(save_state_calls==1 && last_state_slot==1);
    assert(game_session_next_state_slot()==2);
    assert(game_session_next_state_slot()==3);
    assert(game_session_next_state_slot()==1);
    assert(game_session_next_state_slot()==2);
    assert(game_session_load_state()==STATE_IO_EMPTY);
    assert(load_state_calls==1 && last_state_slot==2);
    assert(game_session_set_paused(true));
    assert(game_session_state()==GAME_SESSION_PAUSED);
    assert(game_session_save_state()==STATE_IO_OK);
    assert(save_state_calls==2 && last_state_slot==2);
    game_session_run_frame();
    assert(quadros==1);
    assert(game_session_set_paused(false));
    game_session_run_frame();
    assert(quadros==2);

    game_session_stop();
    assert(game_session_state()==GAME_SESSION_MENU);
    assert(game_session_last_error()==GAME_SESSION_OK);
    assert(game_session_state_slot()==1);
    assert(game_session_save_state()==STATE_IO_UNSUPPORTED);
    assert(!game_session_capabilities()->save_states_available);
    assert(!game_session_capabilities()->cheat_api_present);
    assert(fechamentos==3);
    game_session_stop();
    assert(fechamentos==3);

    assert(inicializacoes==3);
    assert(cargas==2);
    puts("B0: testes de sessao e capacidades passaram");
    return 0;
}
