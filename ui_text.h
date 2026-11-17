#ifndef UI_TEXT_H
#define UI_TEXT_H
#include <stddef.h>
#include <SDL2/SDL.h>
/* Fonte moderna por atlas Lato; compatibilidade com a API existente. */
int ui_texto_init(SDL_Renderer *renderer);
void ui_texto_fechar(void);
int ui_texto_largura(const char *texto,int escala);
int ui_texto_altura(int escala);
void ui_texto_desenhar(const char *texto,int x,int y,int escala,SDL_Color cor,SDL_Color cor_sombra);
void ui_texto_desenhar_centro(const char *texto,int centro_x,int y,int escala,SDL_Color cor,SDL_Color cor_sombra);
void ui_texto_ajustar(const char *texto,int max_largura,int escala,char *saida,size_t capacidade);
#endif
