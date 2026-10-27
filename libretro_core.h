#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H

#include <stdbool.h>

bool init_core(const char* core_path);
bool load_game(const char* game_path);
void run_core(void);
void close_core(void);

#endif