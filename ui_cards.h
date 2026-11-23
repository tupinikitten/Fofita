#ifndef UI_CARDS_H
#define UI_CARDS_H

#include <SDL2/SDL.h>

/* Recorte opcional por jogo, independente das ROMs e dos cartuchos fisicos.
   Zoom em porcentagem (100..250), deslocamentos de -100 a 100. */
typedef struct {
    int zoom_pct;
    int desloc_x;
    int desloc_y;
    int encaixar;
} UiCardAjuste;

/* Ajustes locais persistidos em fofita_cards.cfg, identificados pelo caminho
   da ROM. Carregar uma vez e gravar somente ao confirmar a edicao. */
void ui_cards_carregar_ajustes(void);
void ui_cards_liberar_ajustes(void);
int ui_cards_iniciar_edicao(const char *rom);
int ui_cards_editando(void);
void ui_cards_edicao_mover(int x, int y);
void ui_cards_edicao_zoom(int delta);
void ui_cards_edicao_alternar_modo(void);
void ui_cards_edicao_restaurar(void);
UiCardAjuste ui_cards_edicao_atual(void);
int ui_cards_edicao_salvar(void);
void ui_cards_edicao_cancelar(void);

/* O modo de cartucho nao utiliza estes ajustes. */
void ui_cards_desenhar(SDL_Renderer *renderer, SDL_Texture *capa,
                      const char *titulo, const char *rom,
                      SDL_Rect area, int intensidade, int selecionado);

#endif
