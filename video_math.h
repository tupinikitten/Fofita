#ifndef VIDEO_MATH_H
#define VIDEO_MATH_H
#include <SDL2/SDL.h>
#include <stddef.h>
/* Funcoes puras do motor visual (sem janela nem GPU), cobertas por testes. */

/* Proporcao de exibicao fixa do console; 0 = respeitar o core. */
float video_proporcao_console(const char *id);

/* Retangulo do jogo na janela.
   modo_proporcao: 0 original do console, 1 pixels quadrados.
   escala: 0 automatico, 1 inteira, 2 ajuste livre.
   Retorna 0 se alguma dimensao for invalida. */
int video_calcular_destino(int janela_w, int janela_h, int jogo_w, int jogo_h,
                           const char *console_id, float proporcao_core,
                           int modo_proporcao, int escala, SDL_Rect *destino);

/* Converte um frame (com pitch arbitrario) para RGBA32 compacto.
   Retorna 0 em caso de sucesso. */
int video_frame_para_rgba(const void *origem, int w, int h, int pitch,
                          Uint32 formato, unsigned char *destino);
#endif
