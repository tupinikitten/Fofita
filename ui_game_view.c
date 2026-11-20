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

/* Apresentacao por jogo: se ainda nao existe molde, nao ocultar a capa. */
bool ui_game_view_use_card(bool has_cartridge_template) {
    return modo_atual == GAME_VIEW_CARD || !has_cartridge_template;
}
