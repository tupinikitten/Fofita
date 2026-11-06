#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "video_audio.h"
#include "libretro_core.h"
#include "input.h"
#include "console_registry.h"
#include "ui_carousel.h"
#include "ui_menu.h"

/* true enquanto o menu esta na tela; input.c usa isto para nao repassar teclas ao jogo. */
bool em_menu = true;

/* Carrega o nucleo do console selecionado e a ROM em foco. */
static bool iniciar_jogo_atual(void) {
    const ConsoleConfig *console = ui_menu_console_atual();
    const char *rom = obter_caminho_jogo_atual();

    if (!console || !console->caminho_core || !rom) {
        ui_menu_aviso("Nenhum jogo para iniciar");
        return false;
    }

    if (!init_core(console->caminho_core)) {
        ui_menu_aviso("Falha ao carregar o nucleo do console");
        return false;
    }

    if (!load_game(rom)) {
        printf("Erro: a ROM %s falhou ao abrir.\n", rom);
        close_core();
        ui_menu_aviso("Falha ao abrir a ROM");
        return false;
    }

    descartar_textura_jogo();
    limpar_audio();
    em_menu = false;
    return true;
}

/* Encerra o jogo atual e volta ao menu, sem fechar o aplicativo. */
static void voltar_ao_menu(void) {
    close_core();               /* descarrega o jogo, libera a ROM e o nucleo */
    descartar_textura_jogo();
    limpar_audio();
    em_menu = true;
    ui_menu_focar_carrossel();  /* o cartucho do jogo que acabou de rodar continua em foco */
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!init_video_audio()) return 1;

    init_carousel();

    if (!ui_menu_init()) {
        fechar_carousel();
        fechar_video_audio();
        return 1;
    }

    bool rodando = true;
    SDL_Event event;

    while (rodando) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                rodando = false;
                continue;
            }

            if (event.type != SDL_KEYDOWN) continue;

            SDL_Keycode tecla = event.key.keysym.sym;

            /* Segurar a tecla repete so a navegacao; confirmar/voltar disparam uma vez. */
            if (event.key.repeat &&
                tecla != SDLK_LEFT && tecla != SDLK_RIGHT &&
                tecla != SDLK_UP && tecla != SDLK_DOWN)
                continue;

            if (em_menu) {
                MenuAcao acao = ui_menu_tecla(tecla, event.key.keysym.mod);

                if (acao == MENU_ACAO_INICIAR_JOGO) iniciar_jogo_atual();
                else if (acao == MENU_ACAO_SAIR) rodando = false;
            } else if (tecla == SDLK_SPACE || tecla == SDLK_ESCAPE) {
                voltar_ao_menu();
            }
        }

        SDL_RenderClear(renderer);

        if (em_menu) {
            ui_menu_desenhar();
        } else {
            run_core();
            if (textura_jogo) SDL_RenderCopy(renderer, textura_jogo, NULL, NULL);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (!em_menu) close_core();
    ui_menu_fechar();
    fechar_carousel();
    fechar_video_audio();
    return 0;
}
