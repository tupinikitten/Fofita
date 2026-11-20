#ifndef FOFITA_CORE_PATHS_H
#define FOFITA_CORE_PATHS_H

#include <stdbool.h>

/* root: diretorio persistente e gravavel, criado antes desta chamada.
   No Windows o Fofita o obtem de SDL_GetPrefPath(). */
bool core_paths_init(const char *root);
const char *core_paths_system_dir(void);
const char *core_paths_save_dir(void);
const char *core_paths_state_dir(void);
void core_paths_clear(void);

#endif
