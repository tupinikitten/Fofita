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
