#ifndef UI_CARDS_H
#define UI_CARDS_H

#include <SDL2/SDL.h>

/* Desenha um jogo em formato digital dentro da mesma area do cartucho.
   'capa' pertence ao catalogo; nao e liberada pelo renderizador de cards. */
void ui_cards_desenhar(SDL_Renderer *renderer, SDL_Texture *capa,
                       const char *titulo, SDL_Rect area,
                       int intensidade, int selecionado);

#endif
