#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "video_audio.h"
#include "libretro_core.h"
#include "input.h"
#include "ui_carousel.h" 

bool em_menu = false;

int main(int argc, char *argv[]) {
    if (!init_video_audio()) return 1;
    
    init_carousel(); 

    if (!init_core("cores/fceumm_libretro.dll")) return 1;

    const char* jogo_para_carregar = obter_caminho_jogo_atual();
    
    if (jogo_para_carregar != NULL) {
        // Se a ROM falhar ao abrir, fecha os sistemas e sai sem crash
        if (!load_game(jogo_para_carregar)) {
            printf("Erro crítico: A ROM %s falhou ao abrir.\n", jogo_para_carregar);
            close_core();
            fechar_carousel();
            fechar_video_audio();
            return 1;
        }
    } else {
        printf("Erro: Nenhum jogo .nes foi encontrado na pasta roms/!\n");
        close_core();
        fechar_carousel();
        fechar_video_audio();
        return 1; 
    }

    bool rodando = true;
    SDL_Event event;

    while (rodando) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) rodando = false;
            
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) em_menu = !em_menu; 
                
                if (em_menu) {
                    if (event.key.keysym.sym == SDLK_LEFT)  mover_carousel_esquerda();
                    if (event.key.keysym.sym == SDLK_RIGHT) mover_carousel_direita();
                }
            }
        }
        
        run_core(); 
        SDL_RenderClear(renderer);
        
        if (textura_jogo) SDL_RenderCopy(renderer, textura_jogo, NULL, NULL);
        if (em_menu) desenhar_carousel(); 

        SDL_RenderPresent(renderer); 
        SDL_Delay(16);
    }

    close_core();
    fechar_carousel();
    fechar_video_audio();
    return 0;
}