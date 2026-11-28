/* Menu real do app: percorre pelo teclado ate Configuracoes de video, escolhe
   os shaders, confere a persistencia em fofita_menu.cfg e os padroes.
   Executar numa copia do pacote (usa os assets e grava a configuracao). */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../video_audio.h"
#include "../ui_carousel.h"
#include "../ui_menu.h"
#include "../shader_manager.h"
#include "test_util.h"

bool em_menu = true; /* definido em mini_frontend.c no app */

static void tecla(SDL_Keycode t) {
    VERIFICAR(ui_menu_tecla(t, 0) == MENU_ACAO_NENHUMA, "tecla %d saiu do menu", (int)t);
}

static void desenhar(void) {
    SDL_PumpEvents();
    ui_menu_desenhar();
    SDL_RenderPresent(renderer);
}

static int config_contem(const char *esperado) {
    FILE *f = fopen("fofita_menu.cfg", "r");
    char linha[1024];
    int achou = 0;
    while (f && fgets(linha, sizeof(linha), f)) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (!strcmp(linha, esperado)) achou = 1;
    }
    if (f) fclose(f);
    return achou;
}

int main(void) {
    SDL_SetMainReady();
    remove("fofita_menu.cfg");
    if (!init_video_audio()) { fprintf(stderr, "%s\n", SDL_GetError()); return 2; }
    init_carousel();
    if (!ui_menu_init()) return 2;
    VERIFICAR(video_shader == SHADER_NENHUM, "sem configuracao o shader fica desligado");

    /* Lateral (Configuracoes) -> Video do jogo -> linha do Shader. */
    tecla(SDLK_PAGEDOWN); tecla(SDLK_RETURN);
    tecla(SDLK_DOWN); tecla(SDLK_RETURN);
    for (int i = 0; i < 3; ++i) tecla(SDLK_DOWN);
    desenhar();
    tecla(SDLK_RETURN);
    VERIFICAR(video_shader == SHADER_PASSTHROUGH, "1o toque: %d", video_shader);
    VERIFICAR(config_contem("video_shader=1"), "passthrough nao foi salvo");
    tecla(SDLK_RETURN);
    VERIFICAR(video_shader == SHADER_SLANG_TESTE, "2o toque: %d", video_shader);
    VERIFICAR(config_contem("video_shader=2"), "slang nao foi salvo");
    desenhar();
    tecla(SDLK_RETURN);
    VERIFICAR(video_shader == SHADER_NENHUM, "o ciclo deve voltar a Nenhum: %d", video_shader);
    tecla(SDLK_RETURN); tecla(SDLK_RETURN);

    /* Reabrir o app: a escolha precisa voltar do arquivo. */
    tecla(SDLK_ESCAPE); tecla(SDLK_ESCAPE);
    ui_menu_fechar();
    video_shader = SHADER_NENHUM;
    if (!ui_menu_init()) return 2;
    VERIFICAR(video_shader == SHADER_SLANG_TESTE, "preferencia apos reabrir: %d", video_shader);

    /* Restaurar padroes desliga o shader. */
    tecla(SDLK_PAGEDOWN); tecla(SDLK_RETURN);
    tecla(SDLK_DOWN); tecla(SDLK_RETURN);
    for (int i = 0; i < 4; ++i) tecla(SDLK_DOWN);
    tecla(SDLK_RETURN);
    desenhar();
    VERIFICAR(video_shader == SHADER_NENHUM, "restaurar padroes: %d", video_shader);
    VERIFICAR(config_contem("video_shader=0"), "padrao nao foi salvo");

    ui_menu_fechar();
    fechar_carousel();
    fechar_video_audio();
    remove("fofita_menu.cfg");
    return testes_resumo("test_menu");
}
