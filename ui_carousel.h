#ifndef UI_CAROUSEL_H
#define UI_CAROUSEL_H

#include <stddef.h>
#include <SDL2/SDL.h>
#include "console_registry.h"

void init_carousel(void);
/* Desenha o carrossel centralizado dentro de 'area' (coordenadas da janela). */
void desenhar_carousel(SDL_Rect area);
void mover_carousel_esquerda(void);
void mover_carousel_direita(void);
void fechar_carousel(void);
const char *obter_caminho_jogo_atual(void);

/* Mostra so os jogos de 'console' (NULL = todos) e volta o foco para o primeiro. */
void carousel_definir_console(const ConsoleConfig *console);
size_t carousel_total_visiveis(void);
size_t carousel_posicao_atual(void);
/* Nome do jogo em foco, sem a extensao. Retorna 0 se nao houver jogo. */
int carousel_nome_jogo_atual(char *saida, size_t capacidade);

#endif
