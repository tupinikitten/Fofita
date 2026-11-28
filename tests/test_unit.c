/* Testes sem GPU: proporcao, configuracao, conversao de pixels e falhas
   controladas. Uso: test_unit <dir_temporario> <stub_abi_errada> <stub_incompleto> */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include "../video_math.h"
#include "../video_config.h"
#include "../shader_manager.h"
#include "test_util.h"

static void destino(int jw, int jh, int gw, int gh, const char *id, float core,
                    int modo, int escala, int x, int y, int w, int h) {
    SDL_Rect r = {0};
    int ok = video_calcular_destino(jw, jh, gw, gh, id, core, modo, escala, &r);
    VERIFICAR(ok && r.x == x && r.y == y && r.w == w && r.h == h,
              "%s %dx%d em %dx%d modo=%d escala=%d -> %d,%d %dx%d (esperado %d,%d %dx%d)",
              id ? id : "?", gw, gh, jw, jh, modo, escala, r.x, r.y, r.w, r.h, x, y, w, h);
}

static void testar_proporcao(void) {
    /* Ajuste livre (2): proporcao original do console. */
    destino(1280, 720, 256, 240, "nes", 0, 0, 2, 160, 0, 960, 720);
    destino(1920, 1080, 160, 144, "gb", 0, 0, 2, 360, 0, 1200, 1080);
    destino(1920, 1080, 160, 144, "gbc", 0, 0, 2, 360, 0, 1200, 1080);
    /* Pixels quadrados usam a resolucao nativa. */
    destino(1280, 720, 256, 240, "nes", 0, 1, 2, 256, 0, 768, 720);
    /* Escala inteira. */
    destino(1280, 720, 160, 144, "gb", 0, 0, 1, 240, 0, 800, 720);
    destino(1920, 1080, 160, 144, "gb", 0, 0, 1, 400, 36, 1120, 1008);
    /* Automatico: usa inteira somente se aproveitar ao menos 90%. */
    destino(1920, 1080, 160, 144, "gb", 0, 0, 0, 400, 36, 1120, 1008);
    destino(1000, 800, 256, 240, "nes", 0, 0, 0, 20, 40, 960, 720);
    /* Abaixo de 90% o automatico mantem o ajuste livre. */
    destino(1000, 700, 160, 144, "gb", 0, 0, 0, 111, 0, 778, 700);
    /* Janela estreita: barras em cima e embaixo. */
    destino(600, 900, 256, 240, "nes", 0, 0, 2, 0, 225, 600, 450);
    /* Console desconhecido: usa a proporcao do core, senao a nativa. */
    destino(1000, 500, 100, 100, "arcade", 2.0f, 0, 2, 0, 0, 1000, 500);
    destino(1000, 500, 100, 100, NULL, 0, 0, 2, 250, 0, 500, 500);
    SDL_Rect r;
    VERIFICAR(!video_calcular_destino(0, 720, 160, 144, "gb", 0, 0, 0, &r), "janela 0");
    VERIFICAR(!video_calcular_destino(1280, 720, 160, 0, "gb", 0, 0, 0, &r), "jogo 0");
    /* Janela minima nunca produz dimensao zero. */
    VERIFICAR(video_calcular_destino(1, 1, 256, 240, "nes", 0, 0, 0, &r) &&
              r.w >= 1 && r.h >= 1, "janela 1x1");
}

static void testar_config(const char *dir) {
    VideoPrefs p;
    video_config_padroes(&p);
    VERIFICAR(video_config_ler_linha(&p, "video_escala=2\n") && p.escala == 2, "escala");
    video_config_ler_linha(&p, "video_escala=9\n");
    video_config_ler_linha(&p, "video_escala=abc\n");
    VERIFICAR(p.escala == 2, "escala invalida deve ser ignorada");
    video_config_ler_linha(&p, "video_filtro=1\r\n");
    VERIFICAR(p.filtro == 1, "linha CRLF");
    video_config_ler_linha(&p, "video_proporcao=2\n");
    VERIFICAR(p.proporcao == 0, "4:3 forcado antigo migra para original");
    video_config_ler_linha(&p, "video_proporcao=1");
    VERIFICAR(p.proporcao == 1, "pixels quadrados");
    video_config_ler_linha(&p, "video_shader=1\n");
    VERIFICAR(p.shader == SHADER_PASSTHROUGH, "config antiga de passthrough");
    video_config_ler_linha(&p, "video_shader=2\n");
    VERIFICAR(p.shader == SHADER_SLANG_TESTE, "slang");
    video_config_ler_linha(&p, "video_shader=7\n");
    VERIFICAR(p.shader == SHADER_NENHUM, "shader desconhecido desativa");
    p.shader = 2;
    video_config_ler_linha(&p, "video_shader=lixo\n");
    VERIFICAR(p.shader == SHADER_NENHUM, "shader ilegivel desativa");
    VERIFICAR(!video_config_ler_linha(&p, "oculto=nes\n"), "chave alheia");

    char caminho[1024];
    snprintf(caminho, sizeof(caminho), "%s/cfg_teste.cfg", dir);
    VideoPrefs gravado = {1, 1, 1, SHADER_SLANG_TESTE}, lido;
    FILE *f = fopen(caminho, "w");
    VERIFICAR(f != NULL, "criar %s", caminho);
    if (!f) return;
    VERIFICAR(video_config_gravar(f, &gravado) == 0, "gravar");
    fclose(f);
    video_config_padroes(&lido);
    f = fopen(caminho, "r");
    char linha[256];
    while (f && fgets(linha, sizeof(linha), f)) video_config_ler_linha(&lido, linha);
    if (f) fclose(f);
    remove(caminho);
    VERIFICAR(!memcmp(&gravado, &lido, sizeof(lido)), "ida e volta da configuracao");
}

static void testar_pixels(void) {
    /* Vermelho, verde, azul, branco; pitch com folga (lixo no fim). */
    const unsigned char esperado[16] = {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    unsigned char saida[16];
    Uint16 p1555[2][5], p565[2][5];
    Uint32 p8888[2][5];
    memset(p1555, 0xAB, sizeof(p1555));
    memset(p565, 0xAB, sizeof(p565));
    memset(p8888, 0xAB, sizeof(p8888));
    p1555[0][0]=0x7C00; p1555[0][1]=0x03E0; p1555[1][0]=0x001F; p1555[1][1]=0x7FFF;
    p565[0][0]=0xF800; p565[0][1]=0x07E0; p565[1][0]=0x001F; p565[1][1]=0xFFFF;
    p8888[0][0]=0xFFFF0000u; p8888[0][1]=0xFF00FF00u;
    p8888[1][0]=0xFF0000FFu; p8888[1][1]=0xFFFFFFFFu;
    struct { const void *d; int pitch; Uint32 fmt; const char *nome; } casos[] = {
        {p1555, sizeof(p1555[0]), SDL_PIXELFORMAT_RGB555, "0RGB1555"},
        {p565, sizeof(p565[0]), SDL_PIXELFORMAT_RGB565, "RGB565"},
        {p8888, sizeof(p8888[0]), SDL_PIXELFORMAT_ARGB8888, "XRGB8888"},
    };
    for (int i = 0; i < 3; ++i) {
        memset(saida, 0, sizeof(saida));
        int r = video_frame_para_rgba(casos[i].d, 2, 2, casos[i].pitch, casos[i].fmt, saida);
        /* O SDL pode expandir canais de 5/6 bits com erro de 1 nivel. */
        int iguais = 1;
        for (int k = 0; k < 16; ++k)
            if (abs(saida[k] - esperado[k]) > 1) iguais = 0;
        VERIFICAR(r == 0 && iguais, "conversao %s: %d,%d,%d %d,%d,%d %d,%d,%d %d,%d,%d", casos[i].nome, saida[0],saida[1],saida[2],saida[4],saida[5],saida[6],saida[8],saida[9],saida[10],saida[12],saida[13],saida[14]);
    }
    VERIFICAR(video_frame_para_rgba(NULL,2,2,8,SDL_PIXELFORMAT_RGB565,saida) != 0, "nulo");
    VERIFICAR(video_frame_para_rgba(p565,2,2,2,SDL_PIXELFORMAT_RGB565,saida) != 0, "pitch curto");
    VERIFICAR(video_frame_para_rgba(p565,5000,2,10000,SDL_PIXELFORMAT_RGB565,saida) != 0, "gigante");
}

/* Cada falha deve retornar NULL, marcar o aviso e permitir nova tentativa.
   Todas ocorrem antes de qualquer contexto GPU ser criado. */
static void falha_slang(const char *lib, const char *trecho) {
    Uint16 frame[4] = {0};
    video_shader = SHADER_SLANG_TESTE;
    shader_limpar_falha();
    shader_slang_definir_caminhos(lib, "inexistente.slangp");
    const void *r = shader_slang_processar(frame, 2, 2, 4, SDL_PIXELFORMAT_RGB565, 8, 8);
    VERIFICAR(r == NULL && shader_falhou(), "deveria falhar: %s", lib);
    VERIFICAR(strstr(shader_ultimo_erro(), trecho) != NULL,
              "erro '%s' deveria conter '%s'", shader_ultimo_erro(), trecho);
    VERIFICAR(!shader_slang_processar(frame, 2, 2, 4, SDL_PIXELFORMAT_RGB565, 8, 8),
              "nao deve tentar a cada quadro");
    shader_fechar();
    VERIFICAR(shader_falhou(), "aviso persiste apos fechar o jogo");
    video_shader = SHADER_NENHUM;
    VERIFICAR(!shader_falhou(), "sem shader nao ha aviso");
}

static void testar_falhas(const char *dir, const char *stub_abi, const char *stub_incompleto) {
    Uint16 frame[4] = {0};
    int pitch = 0;
    video_shader = SHADER_NENHUM;
    VERIFICAR(!shader_processar(frame,2,2,4,SDL_PIXELFORMAT_RGB565,&pitch), "desligado");
    VERIFICAR(!shader_slang_processar(frame,2,2,4,SDL_PIXELFORMAT_RGB565,8,8), "desligado slang");

    char ausente[1024], corrompida[1024];
    snprintf(ausente, sizeof(ausente), "%s/nao_existe_librashader", dir);
    falha_slang(ausente, "indisponivel");
    snprintf(corrompida, sizeof(corrompida), "%s/corrompida.dll", dir);
    FILE *f = fopen(corrompida, "wb");
    if (f) { fputs("isto nao e uma biblioteca", f); fclose(f); }
    falha_slang(corrompida, "indisponivel");
    remove(corrompida);
    falha_slang(stub_abi, "incompativel");
    falha_slang(stub_incompleto, "simbolo ausente");
    shader_slang_definir_caminhos(NULL, NULL);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "uso: %s dir stub_abi stub_incompleto\n", argv[0]); return 2; }
    SDL_SetMainReady();
    if (SDL_Init(0) != 0) { fprintf(stderr, "%s\n", SDL_GetError()); return 2; }
    testar_proporcao();
    testar_config(argv[1]);
    testar_pixels();
    testar_falhas(argv[1], argv[2], argv[3]);
    shader_slang_definir_caminhos(NULL, NULL);
    SDL_Quit();
    return testes_resumo("test_unit");
}
