#ifndef VIDEO_AUDIO_H
#define VIDEO_AUDIO_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern SDL_Texture *textura_jogo;
extern int formato_de_cor;

bool init_video_audio(void);
void fechar_video_audio(void);

void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch);
void retro_audio_sample_cb(int16_t left, int16_t right);
size_t retro_audio_sample_batch_cb(const int16_t *data, size_t frames);

#endif