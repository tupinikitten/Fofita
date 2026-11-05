#include "image_processor.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>

/* No modo CAPA_AUTO a arte NUNCA perde conteudo: ela e mostrada inteira,
   sem deformar, e as sobras da etiqueta sao preenchidas prolongando as
   bordas da propria arte. So quando a proporcao e praticamente igual
   (diferenca de ate 5%) o pequeno excesso e recortado. */
#define TOLERANCIA_PROPORCAO_AUTO 0.05f

/* Arredonda os dois cantos inferiores da etiqueta com bordas suaves:
   o alfa de cada pixel vira a cobertura dele dentro do arco (0 a 255). */
static void aplicar_mascara_cantos_inferiores(SDL_Surface *surface, int raio) {
    if (!surface || surface->format->BytesPerPixel != 4 || raio <= 0)
        return;

    int w = surface->w;
    int h = surface->h;

    if (raio > w / 2) raio = w / 2;
    if (raio > h) raio = h;

    if (SDL_LockSurface(surface) != 0)
        return;

    for (int y = h - raio; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float cx;
            if (x < raio) cx = (float)raio;
            else if (x >= w - raio) cx = (float)(w - raio);
            else continue;

            float dx = (x + 0.5f) - cx;
            float dy = (y + 0.5f) - (float)(h - raio);
            float cobertura = (float)raio - sqrtf(dx * dx + dy * dy) + 0.5f;
            if (cobertura >= 1.0f) continue;
            if (cobertura < 0.0f) cobertura = 0.0f;

            Uint32 *pixel = (Uint32 *)(
                (Uint8 *)surface->pixels + y * surface->pitch + x * 4
            );
            Uint8 r, g, b, a;
            SDL_GetRGBA(*pixel, surface->format, &r, &g, &b, &a);
            *pixel = SDL_MapRGBA(surface->format, r, g, b,
                                 (Uint8)(a * cobertura + 0.5f));
        }
    }

    SDL_UnlockSurface(surface);
}

static SDL_Surface *criar_superficie_rgba(int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    return SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
}

/* Recorta 'origem' de 'src' e redimensiona para dw x dh com filtro bilinear.
   Reducoes grandes sao feitas em etapas de metade do tamanho, para evitar
   serrilhado: as artes antigas chegam em resolucoes muito diferentes.
   'src' precisa estar em RGBA32. Devolve uma nova superficie RGBA32. */
static SDL_Surface *escalar_suave(SDL_Surface *src, SDL_Rect origem, int dw, int dh) {
    SDL_Surface *saida = criar_superficie_rgba(dw, dh);
    if (!saida) return NULL;

    SDL_Surface *atual = src;
    SDL_Surface *intermediaria = NULL;

    while (origem.w > 2 * dw || origem.h > 2 * dh) {
        int nw = origem.w > 2 * dw ? origem.w / 2 : origem.w;
        int nh = origem.h > 2 * dh ? origem.h / 2 : origem.h;
        SDL_Surface *prox = criar_superficie_rgba(nw, nh);
        if (!prox || SDL_SoftStretchLinear(atual, &origem, prox, NULL) != 0) {
            if (prox) SDL_FreeSurface(prox);
            if (intermediaria) SDL_FreeSurface(intermediaria);
            SDL_FreeSurface(saida);
            return NULL;
        }
        if (intermediaria) SDL_FreeSurface(intermediaria);
        intermediaria = prox;
        atual = prox;
        origem.x = 0; origem.y = 0; origem.w = prox->w; origem.h = prox->h;
    }

    int ok = SDL_SoftStretchLinear(atual, &origem, saida, NULL) == 0;
    if (intermediaria) SDL_FreeSurface(intermediaria);
    if (!ok) {
        SDL_FreeSurface(saida);
        return NULL;
    }
    return saida;
}

/* Preenche 'barra' (em 'etiqueta') com a cor media de uma faixa da borda da
   arte, reduzida a poucos pixels e esticada: um degrade suave sem listras.
   horizontal=1: barra em cima/embaixo; 0: barra nos lados. */
static void preencher_barra(SDL_Surface *etiqueta, SDL_Surface *original,
                            SDL_Rect faixa, SDL_Rect barra, int horizontal) {
    if (barra.w <= 0 || barra.h <= 0) return;
    SDL_Surface *pequena = horizontal ? escalar_suave(original, faixa, 16, 2)
                                      : escalar_suave(original, faixa, 2, 16);
    if (!pequena) return;
    SDL_Rect toda = {0, 0, pequena->w, pequena->h};
    SDL_Surface *grande = escalar_suave(pequena, toda, barra.w, barra.h);
    SDL_FreeSurface(pequena);
    if (!grande) return;
    SDL_SetSurfaceBlendMode(grande, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(grande, NULL, etiqueta, &barra);
    SDL_FreeSurface(grande);
}

/* Recorte centralizado com a proporcao da etiqueta (w:h). */
static SDL_Rect recorte_centralizado(const SDL_Surface *img, int w, int h) {
    SDL_Rect r = {0, 0, img->w, img->h};
    float proporcao_imagem = (float)img->w / img->h;
    float proporcao_etiqueta = (float)w / h;

    if (proporcao_imagem > proporcao_etiqueta) {
        r.w = (int)(img->h * proporcao_etiqueta + 0.5f);
        if (r.w < 1) r.w = 1;
        r.x = (img->w - r.w) / 2;
    } else if (proporcao_imagem < proporcao_etiqueta) {
        r.h = (int)(img->w / proporcao_etiqueta + 0.5f);
        if (r.h < 1) r.h = 1;
        r.y = (img->h - r.h) / 2;
    }
    return r;
}

SDL_Texture *image_processor_load_cover(
    SDL_Renderer *renderer,
    const ConsoleConfig *console,
    const char *nome_sem_extensao
) {
    if (!renderer || !console || !nome_sem_extensao)
        return NULL;

    char caminho[1024];
    SDL_Surface *carregada = NULL;

    if (console->pasta_capas) {
        snprintf(caminho, sizeof(caminho), "%s/%s.png",
                 console->pasta_capas, nome_sem_extensao);
        carregada = IMG_Load(caminho);
    }

    if (!carregada) {
        snprintf(caminho, sizeof(caminho), "assets/%s.png",
                 nome_sem_extensao);
        carregada = IMG_Load(caminho);
    }

    if (!carregada) {
        printf("Aviso: capa nao encontrada: %s\n", caminho);
        return NULL;
    }

    int w = console->label_w;
    int h = console->label_h;

    if (w <= 0 || h <= 0) {
        SDL_Texture *fallback =
            SDL_CreateTextureFromSurface(renderer, carregada);
        SDL_FreeSurface(carregada);
        return fallback;
    }

    /* Qualquer formato de entrada (RGB, paleta, RGBA) vira RGBA32. */
    SDL_Surface *original =
        SDL_ConvertSurfaceFormat(carregada, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(carregada);
    if (!original || original->w < 1 || original->h < 1) {
        if (original) SDL_FreeSurface(original);
        return NULL;
    }

    SDL_Surface *etiqueta = criar_superficie_rgba(w, h);
    if (!etiqueta) {
        SDL_FreeSurface(original);
        return NULL;
    }
    SDL_FillRect(etiqueta, NULL,
                 SDL_MapRGBA(etiqueta->format, 0, 0, 0, 0));

    ModoEncaixeCapa modo = console->modo_encaixe_capa;
    SDL_Rect inteira = {0, 0, original->w, original->h};
    int arte_inteira = 0; /* 1 = a arte aparece toda, sem recorte */

    if (modo == CAPA_AUTO) {
        float razao = ((float)original->w / original->h) / ((float)w / h);
        if (razao < 1.0f) razao = 1.0f / razao;
        arte_inteira = (razao - 1.0f) > TOLERANCIA_PROPORCAO_AUTO;
        modo = arte_inteira ? CAPA_CONTAIN : CAPA_CROP;
    } else if (modo == CAPA_CONTAIN) {
        arte_inteira = 1;
    }

    SDL_Surface *arte = NULL;
    SDL_Rect destino = {0, 0, w, h};

    if (modo == CAPA_STRETCH) {
        arte = escalar_suave(original, inteira, w, h);
    } else if (modo == CAPA_CROP) {
        arte = escalar_suave(original, recorte_centralizado(original, w, h), w, h);
    } else { /* CAPA_CONTAIN */
        float sx = (float)w / original->w;
        float sy = (float)h / original->h;
        float escala = sx < sy ? sx : sy;

        destino.w = (int)(original->w * escala + 0.5f);
        destino.h = (int)(original->h * escala + 0.5f);
        if (destino.w < 1) destino.w = 1;
        if (destino.h < 1) destino.h = 1;
        destino.x = (w - destino.w) / 2;
        destino.y = (h - destino.h) / 2;
        arte = escalar_suave(original, inteira, destino.w, destino.h);
    }

    /* Automatico + arte inteira: as sobras (barras em cima/embaixo ou nos
       lados) prolongam a cor das bordas da arte, de forma suave. */
    if (console->modo_encaixe_capa == CAPA_AUTO && arte_inteira) {
        int fh = original->h / 60; if (fh < 2) fh = 2;
        int fw = original->w / 60; if (fw < 2) fw = 2;
        if (destino.y > 0) {
            SDL_Rect topo = {0, 0, original->w, fh};
            SDL_Rect base = {0, original->h - fh, original->w, fh};
            SDL_Rect barra_topo = {0, 0, w, destino.y};
            SDL_Rect barra_base = {0, destino.y + destino.h, w, h - (destino.y + destino.h)};
            preencher_barra(etiqueta, original, topo, barra_topo, 1);
            preencher_barra(etiqueta, original, base, barra_base, 1);
        }
        if (destino.x > 0) {
            SDL_Rect esq = {0, 0, fw, original->h};
            SDL_Rect dir = {original->w - fw, 0, fw, original->h};
            SDL_Rect barra_esq = {0, 0, destino.x, h};
            SDL_Rect barra_dir = {destino.x + destino.w, 0, w - (destino.x + destino.w), h};
            preencher_barra(etiqueta, original, esq, barra_esq, 0);
            preencher_barra(etiqueta, original, dir, barra_dir, 0);
        }
    }

    SDL_FreeSurface(original);

    if (arte) {
        SDL_SetSurfaceBlendMode(arte, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(arte, NULL, etiqueta, &destino);
        SDL_FreeSurface(arte);
    }

    aplicar_mascara_cantos_inferiores(
        etiqueta, console->raio_cantos_inferiores);

    SDL_SetSurfaceBlendMode(etiqueta, SDL_BLENDMODE_BLEND);

    SDL_Texture *textura =
        SDL_CreateTextureFromSurface(renderer, etiqueta);

    if (textura)
        SDL_SetTextureBlendMode(textura, SDL_BLENDMODE_BLEND);

    SDL_FreeSurface(etiqueta);
    return textura;
}