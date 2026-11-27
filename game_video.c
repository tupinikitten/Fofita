#include "game_video.h"
#include "video_audio.h"
#include <string.h>

SDL_Texture *textura_jogo = NULL;
int formato_de_cor = SDL_PIXELFORMAT_RGB555;
float proporcao_video = 0.0f;
int video_escala = 0, video_filtro = 0, video_proporcao = 0;

void descartar_textura_jogo(void) {
    if (textura_jogo) {
        SDL_DestroyTexture(textura_jogo);
        textura_jogo = NULL;
    }
}

void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (data) {
        int textura_w = 0, textura_h = 0;
        if (textura_jogo)
            SDL_QueryTexture(textura_jogo, NULL, NULL, &textura_w, &textura_h);

        /* Alguns nucleos podem alterar a resolucao durante a execucao. */
        if (!textura_jogo || textura_w != (int)width || textura_h != (int)height) {
            descartar_textura_jogo();
            textura_jogo = SDL_CreateTexture(renderer, formato_de_cor,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             (int)width, (int)height);
            if (!textura_jogo) return;
            /* Game textures use nearest-neighbor scaling independently of the
               linear filtering used by PNGs and other interface assets. */
#if SDL_VERSION_ATLEAST(2, 0, 12)
            if (SDL_SetTextureScaleMode(textura_jogo, video_filtro ? SDL_ScaleModeLinear : SDL_ScaleModeNearest) != 0)
                SDL_Log("Nao foi possivel ativar escala nearest: %s", SDL_GetError());
#endif
        }
        SDL_UpdateTexture(textura_jogo, NULL, data, (int)pitch);
    }
}

/*
 * Desenha o frame do Libretro sem alterar sua proporcao.
 * Em telas mais largas/altas que o jogo, centraliza a imagem e deixa
 * barras pretas nas areas restantes (letterbox/pillarbox).
 */
static float proporcao_console(const char *id) {
    if (!id) return 0.0f;
    if (!strcmp(id,"gb") || !strcmp(id,"gbc")) return 10.0f/9;
    if (!strcmp(id,"gba")) return 3.0f/2;
    if (!strcmp(id,"psp")) return 30.0f/17;
    if (!strcmp(id,"gamegear")) return 10.0f/9;
    if (!strcmp(id,"nes") || !strcmp(id,"snes") || !strcmp(id,"n64") ||
        !strcmp(id,"ps1") || !strcmp(id,"sms") || !strcmp(id,"genesis") ||
        !strcmp(id,"atari2600") || !strcmp(id,"dreamcast")) return 4.0f/3;
    return 0.0f; /* DS/arcade: respeitar layout do core */
}
void game_video_desenhar(const char *console_id) {
    if (!textura_jogo) return;

    int janela_w = 0, janela_h = 0;
    int jogo_w = 0, jogo_h = 0;
    SDL_GetRendererOutputSize(renderer, &janela_w, &janela_h);
    SDL_QueryTexture(textura_jogo, NULL, NULL, &jogo_w, &jogo_h);

    if (janela_w <= 0 || janela_h <= 0 || jogo_w <= 0 || jogo_h <= 0)
        return;

    float nativa = (float)jogo_w / jogo_h;
    float proporcao = proporcao_console(console_id);
    if (proporcao <= 0.0f) proporcao = proporcao_video > 0.0f ? proporcao_video : nativa;
    if (video_proporcao == 1) proporcao = nativa;
    int destino_w = janela_w;
    int destino_h = (int)(destino_w / proporcao + 0.5f);

    if (destino_h > janela_h) {
        destino_h = janela_h;
        destino_w = (int)(destino_h * proporcao + 0.5f);
    }
    /* Escala inteira vertical, mantendo a proporcao de exibicao.
       Pixels quadrados so sao garantidos quando a proporcao e nativa. */
    int fator = destino_h / jogo_h;
    if (fator >= 1 && (video_escala == 1 ||
        (video_escala == 0 && fator * jogo_h >= destino_h * 0.90f))) {
        destino_h = fator * jogo_h;
        destino_w = (int)(destino_h * proporcao + 0.5f);
    }
    if (destino_w < 1) destino_w = 1;
    if (destino_h < 1) destino_h = 1;
    SDL_Rect destino = {
        (janela_w - destino_w) / 2,
        (janela_h - destino_h) / 2,
        destino_w,
        destino_h
    };

    SDL_RenderCopy(renderer, textura_jogo, NULL, &destino);
}

