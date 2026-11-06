#ifndef UI_TEXT_H
#define UI_TEXT_H

#include <stddef.h>
#include <SDL2/SDL.h>

/*
 * Texto com fonte bitmap 5x7 embutida no proprio programa: nao depende de
 * SDL_ttf nem de arquivos de fonte. Aceita UTF-8; letras acentuadas do
 * Latin-1 sao desenhadas sem o acento (ex.: "ç" -> "c") e qualquer outro
 * caractere fora do ASCII aparece como "?".
 * 'escala' e um inteiro >= 1 (cada pixel da fonte vira escala x escala).
 */
int  ui_texto_init(SDL_Renderer *renderer);
void ui_texto_fechar(void);

int  ui_texto_largura(const char *texto, int escala);
int  ui_texto_altura(int escala);

/* Desenha com uma pequena sombra (cor_sombra) atras do texto. */
void ui_texto_desenhar(const char *texto, int x, int y, int escala,
                       SDL_Color cor, SDL_Color cor_sombra);
void ui_texto_desenhar_centro(const char *texto, int centro_x, int y, int escala,
                              SDL_Color cor, SDL_Color cor_sombra);

/* Copia 'texto' para 'saida' cortando e acrescentando "..." para caber em max_largura. */
void ui_texto_ajustar(const char *texto, int max_largura, int escala,
                      char *saida, size_t capacidade);

#endif
