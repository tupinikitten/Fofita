#include "ui_theme.h"

static const UiTema temas[] = {
    {
        "padrao",
        "assets/ui/wallpapers/bk-01.jpg",
        "assets/ui/estilos/escuro",
        {177, 208, 245, 255},   /* cor_fundo */
        { 28,  38,  66, 255},   /* texto */
        {255, 255, 255, 170},   /* texto_sombra */
        { 62,  78, 120, 255},   /* texto_suave */
        { 28,  38,  66, 255},   /* destaque */
        {255, 255, 255, 150},   /* painel */
        {176,  36,  48, 255}    /* aviso */
    }
};

/* Por enquanto so existe um tema; a escolha do usuario entra aqui depois. */
const UiTema *ui_tema_atual(void) {
    return &temas[0];
}
