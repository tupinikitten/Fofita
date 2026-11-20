#include <assert.h>
#include <stdio.h>
#include "../ui_game_view.h"

int main(void) {
    ui_game_view_set(GAME_VIEW_CARTRIDGE);
    assert(ui_game_view_get()==GAME_VIEW_CARTRIDGE);
    assert(!ui_game_view_use_card(true)); /* NES com molde: visual preservado */
    assert(ui_game_view_use_card(false)); /* GB e GBC sem molde: card */

    ui_game_view_set(GAME_VIEW_CARD);
    assert(ui_game_view_use_card(true));
    assert(ui_game_view_use_card(false));

    ui_game_view_set(GAME_VIEW_CARTRIDGE);
    assert(!ui_game_view_use_card(true)); /* volta ao cartucho quando possivel */
    puts("Fallback digital: testes passaram");
    return 0;
}
