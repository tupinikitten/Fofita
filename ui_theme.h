#ifndef UI_THEME_H
#define UI_THEME_H

#include <SDL2/SDL.h>

/*
 * Tema-base para cores e wallpaper de fallback.
 * O preset de icones e independente do wallpaper, selecionado em ui_menu.c:
 *   ui/styles/light/icons/*_icon_light.png    (claros, fundos escuros)
 *   ui/styles/dark/icons/*_icon_dark.png      (escuros, fundos claros)
 *   ui/styles/colorful/icons/*_icon_colorful.png
 * As setas seguem o contraste ativo. Para a estrutura de assets,
 * consulte ui/styles/README.md.
 */
typedef struct {
    const char *nome;
    const char *wallpaper;          /* caminho do papel de parede */
    const char *pasta_estilo;       /* pasta do estilo de icones, ex.: ui/styles/dark */
    SDL_Color cor_fundo;            /* usado se o wallpaper nao carregar */
    SDL_Color texto;
    SDL_Color texto_sombra;
    SDL_Color texto_suave;          /* contadores e textos secundarios */
    SDL_Color destaque;             /* selecao ativa (borda e botao em foco) */
    SDL_Color painel;               /* fundo translucido de itens e avisos */
    SDL_Color aviso;                /* mensagens de erro */
} UiTema;

const UiTema *ui_tema_atual(void);

#endif
