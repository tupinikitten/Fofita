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

/*
 * Desenha o frame do Libretro sem alterar sua proporcao.
 * Em telas mais largas/altas que o jogo, centraliza a imagem e deixa
 * barras pretas nas areas restantes (letterbox/pillarbox).
 */
static void desenhar_jogo_com_proporcao(void) {
    if (!textura_jogo) return;

    int janela_w = 0, janela_h = 0;
    int jogo_w = 0, jogo_h = 0;
    SDL_GetRendererOutputSize(renderer, &janela_w, &janela_h);
    SDL_QueryTexture(textura_jogo, NULL, NULL, &jogo_w, &jogo_h);

    if (janela_w <= 0 || janela_h <= 0 || jogo_w <= 0 || jogo_h <= 0)
        return;

    float proporcao = proporcao_video > 0.0f
        ? proporcao_video
        : (float)jogo_w / (float)jogo_h;
    int destino_w = janela_w;
    int destino_h = (int)(destino_w / proporcao + 0.5f);

    if (destino_h > janela_h) {
        destino_h = janela_h;
        destino_w = (int)(destino_h * proporcao + 0.5f);
    }
    SDL_Rect destino = {
        (janela_w - destino_w) / 2,
        (janela_h - destino_h) / 2,
        destino_w,
        destino_h
    };

    SDL_RenderCopy(renderer, textura_jogo, NULL, &destino);
}

/* Carrega o nucleo do console selecionado e a ROM em foco. */
static bool iniciar_jogo_atual(void) {
    const ConsoleConfig *console = ui_menu_console_atual();
    const char *rom = obter_caminho_jogo_atual();

    if (!console || !console->caminho_core || !rom) {
        ui_menu_aviso("Nenhum jogo para iniciar");
        return false;
    }

    proporcao_video = 0.0f;
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
    configurar_video_jogo();
    em_menu = false;
    return true;
}

/* Encerra o jogo atual e volta ao menu, sem fechar o aplicativo. */
static void voltar_ao_menu(void) {
    close_core();               /* descarrega o jogo, libera a ROM e o nucleo */
    proporcao_video = 0.0f;
    descartar_textura_jogo();
    limpar_audio();
    configurar_video_menu();
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
    SDL_GameController *controle = NULL;
    Uint32 repeticao_analogica = 0;
    if(SDL_NumJoysticks()>0 && SDL_IsGameController(0))
        controle=SDL_GameControllerOpen(0);
    ui_menu_controle_conectado(controle!=NULL);

    while (rodando) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                rodando = false;
                continue;
            }
            if(event.type==SDL_WINDOWEVENT &&
               (event.window.event==SDL_WINDOWEVENT_RESIZED ||
                event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)) {
                atualizar_video_redimensionado();
                continue;
            }

            if (event.type == SDL_DROPFILE) {
                if (em_menu) ui_menu_wallpaper_solto(event.drop.file);
                SDL_free(event.drop.file);
                continue;
            }
            if(event.type==SDL_CONTROLLERDEVICEADDED) {
                if(!controle && SDL_IsGameController(event.cdevice.which))
                    controle=SDL_GameControllerOpen(event.cdevice.which);
                ui_menu_controle_conectado(controle!=NULL);
                continue;
            }
            if(event.type==SDL_CONTROLLERDEVICEREMOVED) {
                if(controle && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controle))==event.cdevice.which) {
                    SDL_GameControllerClose(controle);
                    controle=NULL;
                }
                ui_menu_controle_conectado(controle!=NULL);
                continue;
            }
            if(event.type==SDL_MOUSEWHEEL) {
                if(em_menu) ui_menu_rolagem(event.wheel.y);
                continue;
            }
            if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT) {
                if(em_menu) {
                    /* Com logical size, mouse chega em pixels da janela.
                       Converter antes da deteccao dos botoes do dashboard. */
                    float lx=0.0f,ly=0.0f;
                    SDL_RenderWindowToLogical(renderer,event.button.x,event.button.y,&lx,&ly);
                    MenuAcao a=ui_menu_clique((int)lx,(int)ly);
                    if(a==MENU_ACAO_INICIAR_JOGO)
                        iniciar_jogo_atual();
                }
                continue;
            }
            SDL_Keycode tecla=SDLK_UNKNOWN;
            Uint16 mod=0;
            int repetir=0;
            if(event.type==SDL_KEYDOWN) {
                tecla=event.key.keysym.sym;
                mod=event.key.keysym.mod;
                repetir=event.key.repeat;
            } else if(event.type==SDL_CONTROLLERBUTTONDOWN) {
                switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: tecla=SDLK_LEFT; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: tecla=SDLK_RIGHT; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: tecla=SDLK_UP; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: tecla=SDLK_DOWN; break;
                case SDL_CONTROLLER_BUTTON_A: tecla=SDLK_RETURN; break;
                case SDL_CONTROLLER_BUTTON_B: tecla=SDLK_ESCAPE; break;
                case SDL_CONTROLLER_BUTTON_START: tecla=SDLK_TAB; break;
                default: break;
                }
            } else if(event.type==SDL_CONTROLLERAXISMOTION && em_menu &&
                     (event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX || event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY)) {
                if(SDL_GetTicks()-repeticao_analogica<170) continue;
                if(event.caxis.value>22000)
                    tecla=event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX?SDLK_RIGHT:SDLK_DOWN;
                else if(event.caxis.value< -22000)
                    tecla=event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX?SDLK_LEFT:SDLK_UP;
                if(tecla!=SDLK_UNKNOWN) repeticao_analogica=SDL_GetTicks();
            }
            if(tecla==SDLK_UNKNOWN) continue;
            if(repetir && tecla!=SDLK_LEFT && tecla!=SDLK_RIGHT &&
               tecla!=SDLK_UP && tecla!=SDLK_DOWN) continue;

            if (em_menu) {
                MenuAcao acao = ui_menu_tecla(tecla, mod);

                if (acao == MENU_ACAO_INICIAR_JOGO)
                    iniciar_jogo_atual();
                else if (acao == MENU_ACAO_SAIR) rodando = false;
            } else if (tecla == SDLK_SPACE || tecla == SDLK_ESCAPE) {
                voltar_ao_menu();
            }
        }

        /* O fundo preto tambem preenche as barras do video em letterbox. */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (em_menu) {
            ui_menu_desenhar();
        } else {
            run_core();
            desenhar_jogo_com_proporcao();
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (controle) SDL_GameControllerClose(controle);
    if (!em_menu) close_core();
    ui_menu_fechar();
    fechar_carousel();
    fechar_video_audio();
    return 0;
}
