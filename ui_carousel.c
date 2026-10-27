#include "ui_carousel.h"
#include "video_audio.h"
#include <SDL2/SDL_image.h>
#include <dirent.h> 
#include <string.h>
#include <stdio.h>
#include <math.h> 

#define MAX_JOGOS 100 

typedef struct {
    char nome_arquivo[256];
    char caminho_completo_rom[512]; 
    SDL_Texture *arte_capa;
} Jogo;

Jogo jogos[MAX_JOGOS];
SDL_Texture *molde_cartucho = NULL; 
int total_jogos = 0;
int foco_atual = 0; 

void init_carousel(void) {
    molde_cartucho = IMG_LoadTexture(renderer, "assets/nes_template.png");
    
    // Lê os jogos diretamente da pasta roms/
    DIR *pasta = opendir("roms/");
    struct dirent *arquivo;

    if (pasta == NULL) {
        printf("Aviso: Pasta roms/ nao encontrada ou vazia.\n");
        return;
    }

    while ((arquivo = readdir(pasta)) != NULL && total_jogos < MAX_JOGOS) {
        char *extensao = strrchr(arquivo->d_name, '.');
        
        if (extensao && strcmp(extensao, ".nes") == 0) {
            
            strncpy(jogos[total_jogos].nome_arquivo, arquivo->d_name, 255);
            
            // Constrói o caminho exato refletindo a sua pasta
            snprintf(jogos[total_jogos].caminho_completo_rom, 512, "roms/%s", arquivo->d_name);
            
            char nome_sem_extensao[256];
            int tamanho_nome = extensao - arquivo->d_name;
            strncpy(nome_sem_extensao, arquivo->d_name, tamanho_nome);
            nome_sem_extensao[tamanho_nome] = '\0';
            
            // Procura a capa na pasta assets/ com o mesmo nome do jogo
            char caminho_capa[512];
            snprintf(caminho_capa, sizeof(caminho_capa), "assets/nes/%s.png", nome_sem_extensao);
            
            jogos[total_jogos].arte_capa = IMG_LoadTexture(renderer, caminho_capa);
            total_jogos++;
        }
    }
    closedir(pasta);
    printf("Total de jogos encontrados: %d\n", total_jogos);
}

const char* obter_caminho_jogo_atual(void) {
    if (total_jogos > 0) return jogos[foco_atual].caminho_completo_rom;
    return NULL;
}

void desenhar_carousel(void) {
    if (total_jogos == 0) return; 

    for (int i = 0; i < total_jogos; i++) {
        SDL_Rect pos_molde;
        int vizinho_esquerda = (foco_atual == 0) ? total_jogos - 1 : foco_atual - 1;
        int vizinho_direita = (foco_atual == total_jogos - 1) ? 0 : foco_atual + 1;

        if (i == vizinho_esquerda || i == vizinho_direita) {
            pos_molde = (i == vizinho_esquerda) ? (SDL_Rect){-20, 160, 280, 280} : (SDL_Rect){540, 160, 280, 280};
            
            if (molde_cartucho) {
                SDL_SetTextureColorMod(molde_cartucho, 100, 100, 100); 
                SDL_RenderCopy(renderer, molde_cartucho, NULL, &pos_molde);
            }
    if (jogos[i].arte_capa) {
                // X+42 centraliza; Y+55 baixa para a reentrância; 196x195 são a largura/altura a 70%
                SDL_Rect pos_arte = { pos_molde.x + 42, pos_molde.y + 55, 196, 195 };
                SDL_SetTextureColorMod(jogos[i].arte_capa, 100, 100, 100); 
                SDL_RenderCopy(renderer, jogos[i].arte_capa, NULL, &pos_arte);
            }
        }
    }

    float tempo = SDL_GetTicks() / 300.0f;
    int flutuacao_y = (int)(sin(tempo) * 10.0f); 

    SDL_Rect pos_molde = {260, 100 + flutuacao_y, 400, 400};
    
    if (molde_cartucho) {
        SDL_SetTextureColorMod(molde_cartucho, 255, 255, 255); 
        SDL_RenderCopy(renderer, molde_cartucho, NULL, &pos_molde);
    }
    if (jogos[foco_atual].arte_capa) {
        // X+60 centraliza na largura de 400px; Y+80 alinha a arte à ranhura superior do plástico NES
        SDL_Rect pos_arte = { pos_molde.x + 60, pos_molde.y + 80, 280, 280 };
        SDL_SetTextureColorMod(jogos[foco_atual].arte_capa, 255, 255, 255); 
        SDL_RenderCopy(renderer, jogos[foco_atual].arte_capa, NULL, &pos_arte); 
    }
}

void mover_carousel_esquerda(void) {
    if (total_jogos > 0) {
        foco_atual--;
        if (foco_atual < 0) foco_atual = total_jogos - 1;
    }
}

void mover_carousel_direita(void) {
    if (total_jogos > 0) {
        foco_atual++;
        if (foco_atual >= total_jogos) foco_atual = 0;
    }
}

void fechar_carousel(void) {
    if (molde_cartucho) SDL_DestroyTexture(molde_cartucho);
    for (int i = 0; i < total_jogos; i++) {
        if (jogos[i].arte_capa) SDL_DestroyTexture(jogos[i].arte_capa);
    }
}