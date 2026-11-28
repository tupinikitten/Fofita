/* Ponta a ponta no Windows: nucleo NES real (cores/fceumm_libretro.dll),
   ROM minima gerada aqui (sem conteudo de terceiros), video do Fofita e
   leitura do que chegou a janela. Executar na pasta do pacote.
   Uso: test_core_nes <dir_tmp> */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../video_audio.h"
#include "../libretro_core.h"
#include "../shader_manager.h"
#include "test_util.h"

bool em_menu = false; /* definido em mini_frontend.c no app */

/* Pinta o fundo com a cor 0x16 da paleta e entra em laco. */
static int gerar_rom(const char *caminho) {
    static const unsigned char codigo[] = {
        0x78, 0xD8, 0xA2, 0xFF, 0x9A,             /* SEI CLD LDX #FF TXS */
        0xE8, 0x8E, 0x00, 0x20, 0x8E, 0x01, 0x20, /* INX; desliga PPU */
        0x2C, 0x02, 0x20, 0x10, 0xFB,             /* espera vblank */
        0x2C, 0x02, 0x20, 0x10, 0xFB,             /* espera vblank */
        0xA9, 0x3F, 0x8D, 0x06, 0x20,             /* endereco $3F00 */
        0xA9, 0x00, 0x8D, 0x06, 0x20,
        0xA9, 0x16, 0x8D, 0x07, 0x20,             /* cor de fundo */
        0xA9, 0x00, 0x8D, 0x06, 0x20, 0x8D, 0x06, 0x20,
        0x8D, 0x05, 0x20, 0x8D, 0x05, 0x20,
        0x2C, 0x02, 0x20, 0x10, 0xFB,
        0xA9, 0x0A, 0x8D, 0x01, 0x20              /* liga o fundo */
    };
    static unsigned char rom[16 + 16384 + 8192];
    memcpy(rom, "NES\x1a\x01\x01", 6);
    unsigned char *prg = rom + 16;
    memset(prg, 0xEA, 16384);
    memcpy(prg, codigo, sizeof(codigo));
    size_t fim = sizeof(codigo);
    unsigned alvo = 0xC000 + (unsigned)fim;
    prg[fim] = 0x4C; prg[fim + 1] = alvo & 255; prg[fim + 2] = alvo >> 8;
    prg[fim + 3] = 0x40; /* RTI para NMI/IRQ */
    unsigned rti = alvo + 3;
    prg[0x3FFA] = rti & 255; prg[0x3FFB] = rti >> 8;
    prg[0x3FFC] = 0x00;      prg[0x3FFD] = 0xC0;
    prg[0x3FFE] = rti & 255; prg[0x3FFF] = rti >> 8;
    FILE *f = fopen(caminho, "wb");
    if (!f) return 0;
    int ok = fwrite(rom, 1, sizeof(rom), f) == sizeof(rom);
    fclose(f);
    return ok;
}

/* Roda o jogo e devolve a imagem apresentada na janela (RGBA). */
static unsigned char *capturar(const char *rom, int modo, int *w, int *h) {
    video_shader = modo;
    shader_limpar_falha();
    if (!init_core("cores/fceumm_libretro.dll") || !load_game(rom)) {
        VERIFICAR(0, "carregar nucleo/ROM");
        return NULL;
    }
    descartar_textura_jogo();
    configurar_video_jogo();
    for (int i = 0; i < 30; ++i) run_core();
    int jw = 0, jh = 0;
    if (!textura_jogo) { VERIFICAR(0, "nucleo nao entregou video"); close_core(); return NULL; }
    SDL_QueryTexture(textura_jogo, NULL, NULL, &jw, &jh);
    *w = jw * 3; *h = jh * 3;
    SDL_SetWindowSize(window, *w, *h);
    SDL_Event evento;
    for (int i = 0; i < 20; ++i) { while (SDL_PollEvent(&evento)) {} SDL_Delay(10); }
    unsigned char *px = malloc((size_t)*w * *h * 4);
    for (int i = 0; i < 60; ++i) {
        SDL_PumpEvents();
        run_core();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        game_video_desenhar("nes");
        if (i == 59) {
            int ow = 0, oh = 0;
            SDL_GetRendererOutputSize(renderer, &ow, &oh);
            VERIFICAR(ow == *w && oh == *h, "janela %dx%d", ow, oh);
            VERIFICAR(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA32,
                                           px, *w * 4) == 0, "%s", SDL_GetError());
        }
        SDL_RenderPresent(renderer);
        limpar_audio();
    }
    VERIFICAR(!shader_falhou(), "shader falhou: %s", shader_ultimo_erro());
    close_core();
    descartar_textura_jogo();
    return px;
}

static int divergentes(const unsigned char *px, int w, int h, const unsigned char *cor,
                       int com_grade) {
    int ruins = 0;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        const double tom[3] = {1.0, 0.9, 0.72};
        double linha = com_grade && y % 3 == 2 ? 0.5 : 1.0;
        double coluna = com_grade && x % 3 == 2 ? 0.75 : 1.0;
        for (int k = 0; k < 3; ++k) {
            int esperado = (int)(cor[k] * (com_grade ? tom[k] * linha * coluna : 1.0) + 0.5);
            if (abs(px[(y * w + x) * 4 + k] - esperado) > 3) { ++ruins; break; }
        }
    }
    return ruins;
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    SDL_SetMainReady();
    char rom[1024];
    snprintf(rom, sizeof(rom), "%s/fundo.nes", argv[1]);
    if (!gerar_rom(rom)) return 2;
    if (!init_video_audio()) { fprintf(stderr, "%s\n", SDL_GetError()); return 2; }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    printf("SDL_Renderer padrao do app: %s\n", info.name);
    video_proporcao = 1;
    video_escala = 2;

    int w = 0, h = 0;
    unsigned char *base = capturar(rom, SHADER_NENHUM, &w, &h);
    unsigned char cor[3] = {0};
    if (base) {
        memcpy(cor, base + ((h / 2) * w + w / 2) * 4, 3);
        printf("nucleo NES %dx%d, cor de fundo %d,%d,%d\n", w / 3, h / 3,
               cor[0], cor[1], cor[2]);
        VERIFICAR(cor[0] > 60 && cor[0] > cor[1] && cor[0] > cor[2], "fundo avermelhado");
        VERIFICAR(divergentes(base, w, h, cor, 0) == 0, "sem shader: imagem uniforme");
    }
    for (int rodada = 0; rodada < 2; ++rodada) {
        int sw = 0, sh = 0;
        unsigned char *s = capturar(rom, SHADER_SLANG_TESTE, &sw, &sh);
        if (s && base && sw == w && sh == h) {
            int ruins = divergentes(s, w, h, cor, 1);
            VERIFICAR(ruins == 0, "slang (sessao %d): %d pixels divergentes", rodada, ruins);
        } else VERIFICAR(0, "sem captura Slang");
        free(s);
    }
    int pw = 0, ph = 0;
    unsigned char *p = capturar(rom, SHADER_PASSTHROUGH, &pw, &ph);
    if (p && pw == w && ph == h)
        VERIFICAR(divergentes(p, w, h, cor, 0) == 0, "passthrough alterou a imagem");
    free(p);
    free(base);
    remove(rom);
    fechar_video_audio();
    return testes_resumo("test_core_nes");
}
