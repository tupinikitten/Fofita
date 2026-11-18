#ifndef UI_GAME_VIEW_H
#define UI_GAME_VIEW_H

/* Opcao de apresentacao: a biblioteca, o foco e o core permanecem os mesmos. */
typedef enum {
    GAME_VIEW_CARTRIDGE = 0,
    GAME_VIEW_CARD = 1
} GameViewMode;

void ui_game_view_set(GameViewMode mode);
GameViewMode ui_game_view_get(void);

#endif
