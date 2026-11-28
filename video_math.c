#include "video_math.h"
#include <string.h>

float video_proporcao_console(const char *id) {
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

/*
 * Mantem a proporcao do jogo. Em telas mais largas/altas que ele, centraliza
 * a imagem e deixa barras pretas (letterbox/pillarbox).
 */
int video_calcular_destino(int janela_w, int janela_h, int jogo_w, int jogo_h,
                           const char *console_id, float proporcao_core,
                           int modo_proporcao, int escala, SDL_Rect *destino) {
    if (!destino || janela_w <= 0 || janela_h <= 0 || jogo_w <= 0 || jogo_h <= 0)
        return 0;

    float nativa = (float)jogo_w / jogo_h;
    float proporcao = video_proporcao_console(console_id);
    if (proporcao <= 0.0f) proporcao = proporcao_core > 0.0f ? proporcao_core : nativa;
    if (modo_proporcao == 1) proporcao = nativa;
    int destino_w = janela_w;
    int destino_h = (int)(destino_w / proporcao + 0.5f);

    if (destino_h > janela_h) {
        destino_h = janela_h;
        destino_w = (int)(destino_h * proporcao + 0.5f);
    }
    /* Escala inteira vertical, mantendo a proporcao de exibicao.
       Pixels quadrados so sao garantidos quando a proporcao e nativa. */
    int fator = destino_h / jogo_h;
    if (fator >= 1 && (escala == 1 ||
        (escala == 0 && fator * jogo_h >= destino_h * 0.90f))) {
        destino_h = fator * jogo_h;
        destino_w = (int)(destino_h * proporcao + 0.5f);
    }
    if (destino_w < 1) destino_w = 1;
    if (destino_h < 1) destino_h = 1;
    destino->x = (janela_w - destino_w) / 2;
    destino->y = (janela_h - destino_h) / 2;
    destino->w = destino_w;
    destino->h = destino_h;
    return 1;
}

int video_frame_para_rgba(const void *origem, int w, int h, int pitch,
                          Uint32 formato, unsigned char *destino) {
    if (!origem || !destino || w <= 0 || h <= 0 || w > 4096 || h > 4096) return -1;
    if (pitch < w * (int)SDL_BYTESPERPIXEL(formato)) return -1;
    return SDL_ConvertPixels(w, h, formato, origem, pitch,
                             SDL_PIXELFORMAT_RGBA32, destino, w * 4);
}
