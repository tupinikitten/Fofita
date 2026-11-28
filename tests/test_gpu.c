/* Execucao REAL na GPU (ou em OpenGL por software no CI): Slang pelo
   librashader, passthrough e o caminho completo game_video -> SDL_Renderer.
   Uso: test_gpu <driver SDL_Renderer> <librashader> <preset.slangp> <dir_tmp>
   Codigo 77: driver de renderizacao indisponivel (teste pulado). */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdlib.h>
#include <string.h>
#include "../shader_manager.h"
#include "../game_video.h"
#include "test_util.h"

SDL_Renderer *renderer; /* usado por game_video.c */
static SDL_Window *janela;
static const char *lib_path, *preset_ok;

typedef struct { int w, h, pitch; Uint32 fmt; unsigned char *dados; } Quadro;

/* Quatro quadrantes distintos: detecta inversao vertical ou horizontal e
   troca de canais. */
static void cor_fonte(int x, int y, int w, int h, int c[3]) {
    int direita = x >= w / 2, baixo = y >= h / 2;
    c[0] = (!direita && !baixo) || (direita && baixo) ? 255 : 0;
    c[1] = (direita && !baixo) || (direita && baixo) ? 255 : 0;
    c[2] = (!direita && baixo) || (direita && baixo) ? 255 : 0;
}

static Quadro criar_quadro(int w, int h, Uint32 fmt) {
    Quadro q = {w, h, (w + 7) * (int)SDL_BYTESPERPIXEL(fmt), fmt, NULL};
    unsigned char *rgba = malloc((size_t)w * h * 4);
    q.dados = malloc((size_t)q.pitch * h);
    memset(q.dados, 0x5A, (size_t)q.pitch * h);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        int c[3];
        cor_fonte(x, y, w, h, c);
        unsigned char *p = rgba + (y * w + x) * 4;
        p[0] = c[0]; p[1] = c[1]; p[2] = c[2]; p[3] = 255;
    }
    SDL_ConvertPixels(w, h, SDL_PIXELFORMAT_RGBA32, rgba, w * 4, fmt, q.dados, q.pitch);
    free(rgba);
    return q;
}

/* modo 0: imagem original ampliada; 1: efeito do preset de teste.
   Retorna a quantidade de pixels fora da tolerancia. */
static int comparar(const unsigned char *px, int sw, int sh, int fw, int fh,
                    int modo, const char *rotulo) {
    int ruins = 0, escala = sw / fw;
    for (int y = 0; y < sh; ++y) for (int x = 0; x < sw; ++x) {
        int c[3];
        cor_fonte(x / escala, y / escala, fw, fh, c);
        double fator[3] = {1, 1, 1};
        if (modo == 1) {
            double linha = ((y % escala) + 0.5) / escala > 0.7 ? 0.5 : 1.0;
            double coluna = ((x % escala) + 0.5) / escala > 0.7 ? 0.75 : 1.0;
            fator[0] = 1.0 * linha * coluna;
            fator[1] = 0.9 * linha * coluna;
            fator[2] = 0.72 * linha * coluna;
        }
        for (int k = 0; k < 3; ++k) {
            int esperado = (int)(c[k] * fator[k] + 0.5);
            if (abs(px[(y * sw + x) * 4 + k] - esperado) > 3) {
                if (!ruins)
                    fprintf(stderr, "%s: 1o desvio em %d,%d canal %d: %d (esperado %d)\n",
                            rotulo, x, y, k, px[(y * sw + x) * 4 + k], esperado);
                ++ruins;
                break;
            }
        }
    }
    return ruins;
}

static void slang_direto(int w, int h, Uint32 fmt, int escala) {
    Quadro q = criar_quadro(w, h, fmt);
    SDL_Window *jan_antes = SDL_GL_GetCurrentWindow();
    SDL_GLContext ctx_antes = SDL_GL_GetCurrentContext();
    const unsigned char *r = shader_slang_processar(q.dados, w, h, q.pitch, fmt,
                                                    w * escala, h * escala);
    VERIFICAR(r != NULL, "slang %dx%d %s: %s", w, h, SDL_GetPixelFormatName(fmt),
              shader_ultimo_erro());
    VERIFICAR(SDL_GL_GetCurrentContext() == ctx_antes &&
              (!ctx_antes || SDL_GL_GetCurrentWindow() == jan_antes),
              "contexto GL anterior nao foi restaurado");
    if (r) {
        char rotulo[120];
        snprintf(rotulo, sizeof(rotulo), "slang %dx%d %s %dx", w, h,
                 SDL_GetPixelFormatName(fmt), escala);
        int ruins = comparar(r, w * escala, h * escala, w, h, 1, rotulo);
        VERIFICAR(ruins == 0, "%s: %d pixels divergentes", rotulo, ruins);
        printf("  ok: %s\n", rotulo);
    }
    free(q.dados);
}

static void testar_falha_preset(const char *preset, const char *rotulo) {
    Quadro q = criar_quadro(160, 144, SDL_PIXELFORMAT_RGB565);
    shader_fechar();
    shader_limpar_falha();
    shader_slang_definir_caminhos(lib_path, preset);
    const void *r = shader_slang_processar(q.dados, q.w, q.h, q.pitch, q.fmt, 640, 576);
    VERIFICAR(r == NULL && shader_falhou() && shader_ultimo_erro()[0],
              "%s deveria falhar com mensagem", rotulo);
    printf("  falha controlada (%s): %s\n", rotulo, shader_ultimo_erro());
    shader_fechar();
    shader_limpar_falha();
    shader_slang_definir_caminhos(lib_path, preset_ok);
    free(q.dados);
}

static void escrever(const char *caminho, const char *texto) {
    FILE *f = fopen(caminho, "wb");
    VERIFICAR(f != NULL, "criar %s", caminho);
    if (f) { fputs(texto, f); fclose(f); }
}

static void testar_slang(const char *dir) {
    video_shader = SHADER_SLANG_TESTE;
    shader_slang_definir_caminhos(lib_path, preset_ok);
    const Uint32 formatos[] = {SDL_PIXELFORMAT_RGB555, SDL_PIXELFORMAT_RGB565,
                               SDL_PIXELFORMAT_ARGB8888};
    for (int i = 0; i < 3; ++i) slang_direto(160, 144, formatos[i], 4);
    slang_direto(160, 144, SDL_PIXELFORMAT_RGB565, 3);  /* muda so a saida */
    slang_direto(256, 240, SDL_PIXELFORMAT_RGB565, 3);  /* muda a entrada */
    slang_direto(160, 144, SDL_PIXELFORMAT_RGB565, 8);

    /* Muitos quadros seguidos: estabilidade e medida de referencia. */
    Quadro q = criar_quadro(256, 240, SDL_PIXELFORMAT_RGB565);
    Uint64 t0 = SDL_GetPerformanceCounter();
    int ok = 0;
    for (int i = 0; i < 700; ++i)
        ok += shader_slang_processar(q.dados, q.w, q.h, q.pitch, q.fmt, 768, 720) != NULL;
    printf("  700 quadros 256x240->768x720: %.2f ms/quadro\n",
           (SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency() / 700);
    VERIFICAR(ok == 700, "quadros processados: %d", ok);
    free(q.dados);

    /* Ciclos de abrir/fechar jogo. */
    for (int i = 0; i < 5; ++i) {
        shader_fechar();
        slang_direto(160, 144, SDL_PIXELFORMAT_RGB565, 4);
    }

    char caminho[1024], shader[1024];
    snprintf(caminho, sizeof(caminho), "%s/nao_existe.slangp", dir);
    testar_falha_preset(caminho, "preset ausente");
    snprintf(caminho, sizeof(caminho), "%s/vazio.slangp", dir);
    escrever(caminho, "isto nao e um preset\n");
    testar_falha_preset(caminho, "preset invalido");
    snprintf(shader, sizeof(shader), "%s/quebrado.slang", dir);
    escrever(shader, "#version 450\n#pragma stage vertex\nvoid main(){ erro de sintaxe }\n"
                     "#pragma stage fragment\nvoid main(){}\n");
    snprintf(caminho, sizeof(caminho), "%s/quebrado.slangp", dir);
    escrever(caminho, "shaders = 1\nshader0 = quebrado.slang\n");
    testar_falha_preset(caminho, "shader com erro de sintaxe");
    slang_direto(160, 144, SDL_PIXELFORMAT_RGB565, 4); /* recuperacao */
}

static void testar_passthrough(void) {
    shader_fechar();
    shader_limpar_falha();
    video_shader = SHADER_PASSTHROUGH;
    const Uint32 formatos[] = {SDL_PIXELFORMAT_RGB555, SDL_PIXELFORMAT_RGB565,
                               SDL_PIXELFORMAT_ARGB8888};
    for (int i = 0; i < 3; ++i) {
        Quadro q = criar_quadro(160, 144, formatos[i]);
        int pitch = 0;
        const void *r = shader_processar(q.dados, q.w, q.h, q.pitch, q.fmt, &pitch);
        VERIFICAR(r != NULL, "passthrough: %s", shader_ultimo_erro());
        if (r) {
            unsigned char *rgba = malloc(160 * 144 * 4);
            SDL_ConvertPixels(160, 144, q.fmt, r, pitch, SDL_PIXELFORMAT_RGBA32, rgba, 640);
            VERIFICAR(comparar(rgba, 160, 144, 160, 144, 0, "passthrough") == 0,
                      "passthrough alterou a imagem");
            free(rgba);
        }
        free(q.dados);
    }
    shader_fechar();
}

/* Processa eventos como o loop principal ate o renderer ver o novo tamanho. */
static void esperar_janela(int w, int h) {
    SDL_Event e;
    int ow = 0, oh = 0;
    for (int i = 0; i < 100; ++i) {
        while (SDL_PollEvent(&e)) {}
        SDL_GetRendererOutputSize(renderer, &ow, &oh);
        if (i >= 3 && ow == w && oh == h) return;
        SDL_Delay(10);
    }
    int jw = 0, jh = 0;
    SDL_GetWindowSize(janela, &jw, &jh);
    SDL_version v;
    SDL_GetVersion(&v);
    fprintf(stderr, "SDL %d.%d.%d: janela %dx%d, saida do renderer %dx%d\n",
            v.major, v.minor, v.patch, jw, jh, ow, oh);
}

/* Desenha como o loop principal e confere o que chegou ao SDL_Renderer. */
static void conferir_tela(int fw, int fh, int escala, int modo, const char *rotulo) {
    int w = fw * escala, h = fh * escala, ow = 0, oh = 0;
    SDL_SetWindowSize(janela, w, h);
    esperar_janela(w, h);
    SDL_GetRendererOutputSize(renderer, &ow, &oh);
    VERIFICAR(ow == w && oh == h, "%s: janela %dx%d (esperado %dx%d)", rotulo, ow, oh, w, h);
    if (ow != w || oh != h) return;
    int falhas_antes = testes_falhos;
    unsigned char *px = malloc((size_t)w * h * 4);
    for (int vez = 0; vez < 2; ++vez) { /* 2a vez: quadro repetido (dupe) */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (vez) retro_video_refresh_cb(NULL, fw, fh, 0);
        game_video_desenhar("gb");
        VERIFICAR(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA32, px, w * 4) == 0,
                  "ler tela: %s", SDL_GetError());
        int ruins = comparar(px, w, h, fw, fh, modo, rotulo);
        VERIFICAR(ruins == 0, "%s (vez %d): %d pixels divergentes", rotulo, vez, ruins);
        SDL_RenderPresent(renderer);
    }
    if (testes_falhos == falhas_antes) printf("  tela ok: %s\n", rotulo);
    free(px);
}

static void enviar_ao_video(int w, int h) {
    Quadro q = criar_quadro(w, h, SDL_PIXELFORMAT_RGB565);
    formato_de_cor = (int)q.fmt;
    retro_video_refresh_cb(q.dados, w, h, (size_t)q.pitch);
    /* O ponteiro do core deixa de valer apos o callback. */
    memset(q.dados, 0, (size_t)q.pitch * h);
    free(q.dados);
}

static void testar_pipeline(const char *dir) {
    video_proporcao = 1; /* pixels quadrados: escala exata */
    video_escala = 2;
    video_filtro = 0;
    shader_slang_definir_caminhos(lib_path, preset_ok);

    descartar_textura_jogo(); shader_limpar_falha();
    video_shader = SHADER_NENHUM;
    enviar_ao_video(160, 144);
    conferir_tela(160, 144, 4, 0, "sem shader");

    descartar_textura_jogo(); shader_limpar_falha();
    video_shader = SHADER_SLANG_TESTE;
    enviar_ao_video(160, 144);
    conferir_tela(160, 144, 4, 1, "slang 4x");
    conferir_tela(160, 144, 3, 1, "slang apos redimensionar");
    enviar_ao_video(256, 240); /* core muda a resolucao durante o jogo */
    conferir_tela(256, 240, 3, 1, "slang apos mudar resolucao");

    descartar_textura_jogo(); shader_limpar_falha();
    char caminho[1024];
    snprintf(caminho, sizeof(caminho), "%s/nao_existe.slangp", dir);
    shader_slang_definir_caminhos(lib_path, caminho);
    enviar_ao_video(160, 144);
    conferir_tela(160, 144, 4, 0, "fallback com preset quebrado");
    VERIFICAR(shader_falhou(), "falha deve ser sinalizada ao menu");

    descartar_textura_jogo(); shader_limpar_falha();
    shader_slang_definir_caminhos(lib_path, preset_ok);
    enviar_ao_video(160, 144);
    conferir_tela(160, 144, 4, 1, "slang no jogo seguinte");

    descartar_textura_jogo(); shader_limpar_falha();
    video_shader = SHADER_PASSTHROUGH;
    enviar_ao_video(160, 144);
    conferir_tela(160, 144, 4, 0, "passthrough na tela");
    descartar_textura_jogo();
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "uso: %s driver lib preset dir_tmp\n", argv[0]); return 2; }
    const char *driver = argv[1];
    lib_path = argv[2];
    preset_ok = argv[3];
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "%s\n", SDL_GetError()); return 2; }
    int indice = -1;
    for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        SDL_RendererInfo info;
        if (!SDL_GetRenderDriverInfo(i, &info) && !strcmp(info.name, driver)) indice = i;
    }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver);
    janela = SDL_CreateWindow("teste", 40, 40, 640, 576, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (janela && indice >= 0) renderer = SDL_CreateRenderer(janela, indice, 0);
    SDL_RendererInfo info;
    if (!renderer || SDL_GetRendererInfo(renderer, &info) || strcmp(info.name, driver)) {
        printf("PULADO: renderer %s indisponivel (%s)\n", driver, SDL_GetError());
        return 77;
    }
    printf("SDL_Renderer: %s\n", info.name);
    testar_slang(argv[4]);
    testar_passthrough();
    testar_pipeline(argv[4]);
    shader_slang_definir_caminhos(NULL, NULL);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(janela);
    SDL_Quit();
    return testes_resumo("test_gpu");
}
