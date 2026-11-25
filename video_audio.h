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
extern int video_escala; /* 0 automatico, 1 inteira, 2 ajuste livre */
extern int video_filtro; /* 0 nitido, 1 suave */
extern int video_proporcao; /* 0 console, 1 pixels quadrados, 2 4:3 */
extern float proporcao_video; /* aspect ratio informado pelo core Libretro, se disponivel */

/* Dashboard: layout em coordenadas 1280x720, projetado uniformemente pelo SDL
   nas resolucoes de saida 720p/1080p. A referencia visual 1920x1080
   continua possivel sem aplicar escala extra ao renderer. */
#define FOFITA_CANVAS_W 1920
#define FOFITA_CANVAS_H 1080
#define FOFITA_UI_W 1280
#define FOFITA_UI_H 720

void configurar_video_menu(void);
void configurar_video_jogo(void);
void atualizar_video_redimensionado(void);
void obter_tamanho_ui(int *w, int *h);

bool init_video_audio(void);
void fechar_video_audio(void);
/* Chamadas ao voltar ao menu: o proximo jogo pode ter outro tamanho/formato de video. */
void descartar_textura_jogo(void);
void limpar_audio(void);

/* Efeitos de interface em dispositivo independente do audio Libretro. */
typedef enum { UI_SOM_NAVEGACAO, UI_SOM_TROCA } UiSom;
void ui_som_tocar(UiSom som);

void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch);
void retro_audio_sample_cb(int16_t left, int16_t right);
size_t retro_audio_sample_batch_cb(const int16_t *data, size_t frames);

#endif