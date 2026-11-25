#ifndef UI_WALLPAPER_H
#define UI_WALLPAPER_H

#include <SDL2/SDL.h>

/* Mantem apenas o wallpaper em uso. GIF animado requer SDL2_image >=2.6.
   Em caso de falta de suporte ou arquivo pesado, usa o primeiro quadro. */
int ui_wallpaper_carregar(SDL_Renderer *renderer,const char *caminho,
                          int animacoes_ativas);
SDL_Texture *ui_wallpaper_textura(void);
int ui_wallpaper_esta_animado(void);
void ui_wallpaper_fechar(void);

#endif
