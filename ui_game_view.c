#include "ui_game_view.h"

/* Um unico estado de apresentacao, sem duplicar a lista dos jogos. */
static GameViewMode modo_atual = GAME_VIEW_CARTRIDGE;

void ui_game_view_set(GameViewMode mode) {
    if(mode != GAME_VIEW_CARD) mode = GAME_VIEW_CARTRIDGE;
    modo_atual = mode;
}
GameViewMode ui_game_view_get(void) {
    return modo_atual;
}

/* Tres alternativas do mesmo catalogo; o foco nao e reinicializado. */
static CardLayout layout_cards_atual = CARD_LAYOUT_3;
void ui_card_layout_set(CardLayout layout) {
    if(layout!=CARD_LAYOUT_5 && layout!=CARD_LAYOUT_GRID)
        layout=CARD_LAYOUT_3;
    layout_cards_atual=layout;
}
CardLayout ui_card_layout_get(void) { return layout_cards_atual; }
