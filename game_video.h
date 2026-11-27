#ifndef GAME_VIDEO_H
#define GAME_VIDEO_H
#include <SDL2/SDL.h>
#include <stddef.h>
extern SDL_Texture *textura_jogo;
extern int formato_de_cor;
extern int video_escala, video_filtro, video_proporcao;
extern float proporcao_video;
void descartar_textura_jogo(void);
void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch);
void game_video_desenhar(const char *console_id);
#endif
