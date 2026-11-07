#ifndef UI_THEME_H
#define UI_THEME_H

#include <SDL2/SDL.h>

/*
 * Tudo que muda a aparencia do menu fica aqui, e nao espalhado pelo codigo:
 * o papel de parede, a pasta do estilo de icones e as cores.
 *
 * Estrutura de assets (preparada para novos estilos e wallpapers):
 *   ui/wallpapers/<arquivo>        papeis de parede (jpg/png)
 *   ui/consoles/<id>_icon.png      icones dos consoles (coloridos, valem p/ todos os estilos)
 *   ui/styles/<estilo>/arrows/    setas
 *   ui/styles/<estilo>/icons/     icones da barra lateral (trofeu, engrenagem, ...)
 * Hoje existe so o estilo "escuro" (icones pretos, para fundos claros). Um estilo
 * "claro" ou "colorido" e uma nova pasta em ui/styles com os mesmos nomes
 * de arquivo, mais uma entrada nova na lista de temas em ui_theme.c.
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
