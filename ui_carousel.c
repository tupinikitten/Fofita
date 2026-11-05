#include "ui_carousel.h"
#include "video_audio.h"
#include "console_registry.h"
#include "image_processor.h"
#include <SDL2/SDL_image.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>

#define INITIAL_GAME_CAPACITY 64
#define MAX_PATH_SIZE 1024

typedef struct {
    char nome_arquivo[256];
    char caminho_completo_rom[MAX_PATH_SIZE];
    SDL_Texture *arte_capa;
    const ConsoleConfig *sistema;
} Jogo;

typedef struct {
    const ConsoleConfig *config;
    SDL_Texture *textura_molde;
} ConsoleRuntime;

static Jogo *jogos = NULL;
static size_t total_jogos = 0;
static size_t capacidade_jogos = 0;
static size_t foco_atual = 0;
static ConsoleRuntime *runtime_consoles = NULL;
static size_t total_consoles = 0;

static int caminho_e_diretorio(const char *path) {
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static int adicionar_jogo(const char *nome, const char *caminho, const ConsoleConfig *console) {
    if (!nome || !caminho || !console || total_jogos >= (size_t)-1 / sizeof(Jogo)) return 0;

    if (total_jogos == capacidade_jogos) {
        size_t nova_capacidade = capacidade_jogos ? capacidade_jogos * 2 : INITIAL_GAME_CAPACITY;
        Jogo *novo = (Jogo *)realloc(jogos, nova_capacidade * sizeof(Jogo));

        if (!novo) {
            fprintf(stderr, "Erro: memoria insuficiente ao ampliar a biblioteca de jogos.\n");
            return 0;
        }

        jogos = novo;
        capacidade_jogos = nova_capacidade;
    }

    Jogo *jogo = &jogos[total_jogos];
    memset(jogo, 0, sizeof(*jogo));

    snprintf(jogo->nome_arquivo, sizeof(jogo->nome_arquivo), "%s", nome);
    snprintf(jogo->caminho_completo_rom, sizeof(jogo->caminho_completo_rom), "%s", caminho);
    jogo->sistema = console;

    const char *ext = strrchr(nome, '.');
    size_t base_len = ext ? (size_t)(ext - nome) : strlen(nome);

    char base[256];
    if (base_len >= sizeof(base)) base_len = sizeof(base) - 1;

    memcpy(base, nome, base_len);
    base[base_len] = '\0';

    jogo->arte_capa = image_processor_load_cover(renderer, console, base);

    total_jogos++;
    return 1;
}

static int ja_cadastrado(const char *caminho) {
    for (size_t i = 0; i < total_jogos; ++i) {
        if (strcmp(jogos[i].caminho_completo_rom, caminho) == 0)
            return 1;
    }

    return 0;
}

static void escanear_pasta_console(const ConsoleConfig *console, const char *pasta_path) {
    DIR *dir = opendir(pasta_path);
    if (!dir) return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char caminho[MAX_PATH_SIZE];
        snprintf(caminho, sizeof(caminho), "%s/%s", pasta_path, entry->d_name);

        if (caminho_e_diretorio(caminho)) continue;

        const char *ext = strrchr(entry->d_name, '.');
        if (!ext) continue;

        const ConsoleConfig *detectado = console_registry_find_by_extension(ext);
        if (!detectado) continue;

        if (strcmp(pasta_path, "roms") != 0 && detectado != console) {
            int extensao_pertence = 0;

            for (size_t k = 0; k < console->quantidade_extensoes; ++k) {
                if (strlen(ext) == strlen(console->extensoes[k])) {
                    int igual = 1;

                    for (size_t z = 0; ext[z]; ++z) {
                        char a = ext[z];
                        char b = console->extensoes[k][z];

                        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');

                        if (a != b) {
                            igual = 0;
                            break;
                        }
                    }

                    if (igual) extensao_pertence = 1;
                }
            }

            if (!extensao_pertence) continue;
        }

        if (strcmp(pasta_path, "roms") == 0 && ja_cadastrado(caminho))
            continue;

        adicionar_jogo(entry->d_name, caminho, console);
    }

    closedir(dir);
}

void init_carousel(void) {
    const ConsoleConfig *configs = console_registry_get_all(&total_consoles);

    runtime_consoles = (ConsoleRuntime *)calloc(total_consoles, sizeof(ConsoleRuntime));

    if (!runtime_consoles) {
        total_consoles = 0;
        fprintf(stderr, "Erro: nao foi possivel preparar os consoles.\n");
        return;
    }

    for (size_t i = 0; i < total_consoles; ++i) {
        runtime_consoles[i].config = &configs[i];

        if (configs[i].caminho_molde) {
            runtime_consoles[i].textura_molde =
                IMG_LoadTexture(renderer, configs[i].caminho_molde);

            if (!runtime_consoles[i].textura_molde) {
                printf("Aviso: molde %s nao encontrado.\n", configs[i].caminho_molde);
            } else {
                int tw = 0, th = 0;
                SDL_QueryTexture(runtime_consoles[i].textura_molde, NULL, NULL, &tw, &th);
                if (tw != configs[i].molde_w || th != configs[i].molde_h)
                    printf("Aviso: molde %s tem %dx%d, mas o registro diz %dx%d. "
                           "Confira as coordenadas da etiqueta.\n",
                           configs[i].caminho_molde, tw, th,
                           configs[i].molde_w, configs[i].molde_h);
            }
        }
    }

    DIR *root = opendir("roms");

    if (root) {
        struct dirent *entry;

        while ((entry = readdir(root)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            char caminho[MAX_PATH_SIZE];
            snprintf(caminho, sizeof(caminho), "roms/%s", entry->d_name);

            if (caminho_e_diretorio(caminho)) continue;

            const char *ext = strrchr(entry->d_name, '.');
            const ConsoleConfig *console =
                ext ? console_registry_find_by_extension(ext) : NULL;

            if (console)
                adicionar_jogo(entry->d_name, caminho, console);
        }

        closedir(root);
    }

    for (size_t i = 0; i < total_consoles; ++i)
        escanear_pasta_console(&configs[i], configs[i].pasta_roms);

    printf("Total de jogos encontrados: %zu\n", total_jogos);
}

const char *obter_caminho_jogo_atual(void) {
    return total_jogos > 0
        ? jogos[foco_atual].caminho_completo_rom
        : NULL;
}

static void desenhar_jogo(size_t index, SDL_Rect pos_molde, int intensidade) {
    const ConsoleConfig *sis = jogos[index].sistema;
    SDL_Texture *molde = NULL;

    for (size_t i = 0; i < total_consoles; ++i) {
        if (runtime_consoles[i].config == sis) {
            molde = runtime_consoles[i].textura_molde;
            break;
        }
    }

    /* Tamanho de referencia do molde: o do PNG carregado (real). O valor do
       registro so e usado se nao houver textura, assim um molde_w/molde_h
       errado no registro nao desloca mais a etiqueta. */
    int ref_w = sis->molde_w;
    int ref_h = sis->molde_h;

    if (molde) {
        int molde_tex_w = 0;
        int molde_tex_h = 0;

        SDL_QueryTexture(molde, NULL, NULL, &molde_tex_w, &molde_tex_h);

        if (molde_tex_w > 0 && molde_tex_h > 0) {
            ref_w = molde_tex_w;
            ref_h = molde_tex_h;
        }

        if (molde_tex_w > 0 && molde_tex_h > 0 &&
            pos_molde.w > 0 && pos_molde.h > 0) {

            float sx = (float)pos_molde.w / (float)molde_tex_w;
            float sy = (float)pos_molde.h / (float)molde_tex_h;
            float escala_molde = sx < sy ? sx : sy;

            int render_w = (int)(molde_tex_w * escala_molde + 0.5f);
            int render_h = (int)(molde_tex_h * escala_molde + 0.5f);

            pos_molde.x += (pos_molde.w - render_w) / 2;
            pos_molde.y += (pos_molde.h - render_h) / 2;
            pos_molde.w = render_w;
            pos_molde.h = render_h;
        }

        SDL_SetTextureColorMod(molde, (Uint8)intensidade, (Uint8)intensidade, (Uint8)intensidade);
        SDL_RenderCopy(renderer, molde, NULL, &pos_molde);
        SDL_SetTextureColorMod(molde, 255, 255, 255);
    }

    if (jogos[index].arte_capa &&
        sis->label_w > 0 && sis->label_h > 0 &&
        ref_w > 0 && ref_h > 0) {

        float escala_x = (float)pos_molde.w / (float)ref_w;
        float escala_y = (float)pos_molde.h / (float)ref_h;

        /* Bordas arredondadas (e nao truncadas) para nao abrir frestas de 1 px. */
        int x0 = pos_molde.x + (int)lroundf(sis->off_x * escala_x);
        int y0 = pos_molde.y + (int)lroundf(sis->off_y * escala_y);
        int x1 = pos_molde.x + (int)lroundf((sis->off_x + sis->label_w) * escala_x);
        int y1 = pos_molde.y + (int)lroundf((sis->off_y + sis->label_h) * escala_y);

        SDL_Rect pos_arte = { x0, y0, x1 - x0, y1 - y0 };

        SDL_SetTextureColorMod(jogos[index].arte_capa, (Uint8)intensidade, (Uint8)intensidade, (Uint8)intensidade);
        SDL_RenderCopy(renderer, jogos[index].arte_capa, NULL, &pos_arte);
        SDL_SetTextureColorMod(jogos[index].arte_capa, 255, 255, 255);
    }
}

void desenhar_carousel(void) {
    if (total_jogos == 0) return;

    int viewport_w = 800;
    int viewport_h = 600;

    if (renderer)
        SDL_GetRendererOutputSize(renderer, &viewport_w, &viewport_h);

    if (viewport_w <= 0) viewport_w = 800;
    if (viewport_h <= 0) viewport_h = 600;

    const int cart_box_w = 400;
    const int cart_box_h = 400;
    const int side_box_w = 280;
    const int side_box_h = 280;

    const int center_x = viewport_w / 2;
    const int center_y = viewport_h / 2;
    const int main_x = center_x - cart_box_w / 2;
    const int main_y = center_y - cart_box_h / 2;

    if (total_jogos > 1) {
        size_t esquerda = foco_atual == 0 ? total_jogos - 1 : foco_atual - 1;
        size_t direita = foco_atual + 1 == total_jogos ? 0 : foco_atual + 1;

        desenhar_jogo(esquerda,
            (SDL_Rect){center_x - 300 - side_box_w / 2,
                       center_y - side_box_h / 2, side_box_w, side_box_h}, 100);

        desenhar_jogo(direita,
            (SDL_Rect){center_x + 300 - side_box_w / 2,
                       center_y - side_box_h / 2, side_box_w, side_box_h}, 100);
    }

    float tempo = SDL_GetTicks() / 300.0f;
    int flutuacao_y = (int)(sin(tempo) * 10.0f);

    desenhar_jogo(foco_atual,
        (SDL_Rect){main_x, main_y + flutuacao_y, cart_box_w, cart_box_h}, 255);
}

void mover_carousel_esquerda(void) {
    if (total_jogos > 0)
        foco_atual = foco_atual == 0 ? total_jogos - 1 : foco_atual - 1;
}

void mover_carousel_direita(void) {
    if (total_jogos > 0)
        foco_atual = (foco_atual + 1) % total_jogos;
}

void fechar_carousel(void) {
    for (size_t i = 0; i < total_consoles; ++i) {
        if (runtime_consoles && runtime_consoles[i].textura_molde)
            SDL_DestroyTexture(runtime_consoles[i].textura_molde);
    }

    for (size_t i = 0; i < total_jogos; ++i) {
        if (jogos[i].arte_capa)
            SDL_DestroyTexture(jogos[i].arte_capa);
    }

    free(runtime_consoles);
    free(jogos);

    runtime_consoles = NULL;
    jogos = NULL;
    total_consoles = total_jogos = capacidade_jogos = foco_atual = 0;
}