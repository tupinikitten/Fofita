#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <SDL2/SDL.h>
#include "console_registry.h"

SDL_Texture *image_processor_load_cover(
    SDL_Renderer *renderer,
    const ConsoleConfig *console,
    const char *nome_sem_extensao
);

#endif