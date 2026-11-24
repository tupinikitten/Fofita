#ifndef UI_GAME_VIEW_H
#define UI_GAME_VIEW_H

/* Opcao de apresentacao: a biblioteca, o foco e o core permanecem os mesmos. */
typedef enum {
    GAME_VIEW_CARTRIDGE = 0,
    GAME_VIEW_CARD = 1
} GameViewMode;

void ui_game_view_set(GameViewMode mode);
GameViewMode ui_game_view_get(void);

/* Opcao separada, aplicada SOMENTE ao modo digital (nao aos cartuchos). */
typedef enum {
    CARD_LAYOUT_3 = 0,
    CARD_LAYOUT_5 = 1,
    CARD_LAYOUT_GRID = 2
} CardLayout;
void ui_card_layout_set(CardLayout layout);
CardLayout ui_card_layout_get(void);

#endif
