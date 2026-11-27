#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H
#include <SDL2/SDL.h>
/* Experimental offscreen GPU pass. Returns NULL on failure or when disabled. */
const void *shader_processar(const void *data, int w, int h, int pitch, Uint32 format, int *out_pitch);
void shader_fechar(void);
#endif
