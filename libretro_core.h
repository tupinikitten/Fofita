#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H

#include <stdbool.h>
#include <stddef.h>

/* Snapshot depois de retro_load_game. API de cheats presente nao garante
   que um determinado codigo de cheat sera aceito pelo nucleo. */
typedef struct {
    bool save_states_available;
    size_t save_state_bytes;
    bool save_ram_available;
    size_t save_ram_bytes;
    bool cheat_api_present;
} CoreCapabilities;

const CoreCapabilities *core_get_capabilities(void);
bool init_core(const char* core_path);
bool load_game(const char* game_path);
void run_core(void);
void unload_game(void);
void close_core(void);

#endif