#include "ui_carousel.h"
#include "ui_game_view.h"
#include "ui_cards.h"
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
    SDL_Texture *arte_card; /* Arte quadrada opcional, independente do cartucho. */
    const ConsoleConfig *sistema;
} Jogo;

typedef struct {
    const ConsoleConfig *config;
    SDL_Texture *textura_molde;
} ConsoleRuntime;

static Jogo *jogos = NULL;
static size_t total_jogos = 0;
static size_t capacidade_jogos = 0;
static size_t foco_atual = 0;       /* posicao dentro de 'indices' (jogos visiveis) */
static size_t *indices = NULL;        /* indices em 'jogos' dos jogos do console filtrado */
static size_t total_visiveis = 0;
static const ConsoleConfig *console_filtro = NULL;
static ConsoleRuntime *runtime_consoles = NULL;
static size_t total_consoles = 0;
static int colunas_grade_atual = 5;

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
    /* labels/<console>/cards/<ROM sem extensao>.png, com fallback legado.
       Nomes iguais sao esperados: as pastas distinguem as duas artes.
       O carregamento tambem pode ser repetido depois de copiar novos PNGs. */
    char caminho_card[MAX_PATH_SIZE];
    int comprimento=snprintf(caminho_card,sizeof(caminho_card),
                             "labels/%s/cards/%s.png",console->id,base);
    if(comprimento>0 && (size_t)comprimento<sizeof(caminho_card))
        jogo->arte_card=IMG_LoadTexture(renderer,caminho_card);
    if(!jogo->arte_card) {
        comprimento=snprintf(caminho_card,sizeof(caminho_card),
                             "labels/cards/%s/%s.png",console->id,base);
        if(comprimento>0 && (size_t)comprimento<sizeof(caminho_card))
            jogo->arte_card=IMG_LoadTexture(renderer,caminho_card);
    }

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
    ui_cards_carregar_ajustes();
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
    carousel_definir_console(NULL);
}

static void reconstruir_indices(void) {
    free(indices);
    indices = total_jogos > 0 ? (size_t *)malloc(total_jogos * sizeof(size_t)) : NULL;
    total_visiveis = 0;
    foco_atual = 0;

    if (!indices) return;

    for (size_t i = 0; i < total_jogos; ++i)
        if (!console_filtro || jogos[i].sistema == console_filtro)
            indices[total_visiveis++] = i;
}

void carousel_definir_console(const ConsoleConfig *console) {
    console_filtro = console;
    reconstruir_indices();
}

size_t carousel_total_visiveis(void) { return total_visiveis; }

/* Recarrega somente capas digitais do console atual, sem mexer no foco,
   nas ROMs, nas etiquetas fisicas nem no cartucho selecionado. */
void carousel_recarregar_cards(void) {
    for(size_t i=0;i<total_jogos;++i) {
        Jogo *j=&jogos[i];
        if(console_filtro && j->sistema!=console_filtro) continue;
        const char *ext=strrchr(j->nome_arquivo,'.');
        size_t len=ext?(size_t)(ext-j->nome_arquivo):strlen(j->nome_arquivo);
        if(len>=sizeof(j->nome_arquivo)) continue;
        char base[sizeof(j->nome_arquivo)];
        memcpy(base,j->nome_arquivo,len);
        base[len]='\0';
        char path[MAX_PATH_SIZE];
        SDL_Texture *nova=NULL;
        int n=snprintf(path,sizeof(path),"labels/%s/cards/%s.png",
                       j->sistema->id,base);
        if(n>0 && (size_t)n<sizeof(path))
            nova=IMG_LoadTexture(renderer,path);
        if(!nova) {
            n=snprintf(path,sizeof(path),"labels/cards/%s/%s.png",
                       j->sistema->id,base);
            if(n>0 && (size_t)n<sizeof(path))
                nova=IMG_LoadTexture(renderer,path);
        }
        if(j->arte_card) SDL_DestroyTexture(j->arte_card);
        j->arte_card=nova;
    }
}


/* A contagem nao inclui etiquetas fisicas utilizadas como fallback. */
size_t carousel_contar_cards_exclusivos(void) {
    size_t quantidade=0;
    for(size_t i=0;i<total_jogos;++i) {
        if(console_filtro && jogos[i].sistema!=console_filtro) continue;
        if(jogos[i].arte_card) ++quantidade;
    }
    return quantidade;
}

size_t carousel_posicao_atual(void) { return foco_atual; }

int carousel_nome_jogo_atual(char *saida, size_t capacidade) {
    if (!saida || capacidade == 0) return 0;
    saida[0] = '\0';
    if (total_visiveis == 0) return 0;

    const char *nome = jogos[indices[foco_atual]].nome_arquivo;
    const char *ext = strrchr(nome, '.');
    size_t len = ext ? (size_t)(ext - nome) : strlen(nome);

    if (len >= capacidade) len = capacidade - 1;
    memcpy(saida, nome, len);
    saida[len] = '\0';
    return 1;
}

const char *obter_caminho_jogo_atual(void) {
    return total_visiveis > 0
        ? jogos[indices[foco_atual]].caminho_completo_rom
        : NULL;
}

static void desenhar_jogo(size_t index, SDL_Rect pos_molde, int intensidade) {
    /* Mesma selecao e mesma lista de jogos: somente troca o renderizador. */
    if(ui_game_view_get()==GAME_VIEW_CARD) {
        ui_cards_desenhar(renderer,
                          jogos[index].arte_card ? jogos[index].arte_card
                                                 : jogos[index].arte_capa,
                          jogos[index].nome_arquivo,
                          jogos[index].caminho_completo_rom,
                          pos_molde,intensidade,intensidade==255);
        return;
    }
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

/* Geometria compartilhada entre desenho, mouse e navegacao da grade. */
static void grade_geometria(SDL_Rect area,int *colunas,int *lado,
                            int *gap_x,int *gap_y,int *y0) {
    int cols=area.w>=860?5:(area.w>=650?4:3);
    colunas_grade_atual=cols;
    int gx=20,gy=20;
    int sz=(area.w-30-(cols-1)*gx)/cols;
    int max_h=(area.h-54-gy)/2;
    if(sz>max_h) sz=max_h;
    if(sz>174) sz=174;
    if(sz<32) sz=32;
    if(colunas) *colunas=cols;
    if(lado) *lado=sz;
    if(gap_x) *gap_x=gx;
    if(gap_y) *gap_y=gy;
    if(y0) *y0=area.y+(area.h-(2*sz+gy))/2;
}
static SDL_Rect grade_celula(SDL_Rect area,int cols,int lado,
                             int gap_x,int gap_y,int y0,
                             size_t pagina_inicio,size_t pos) {
    const int linha=(int)(pos/(size_t)cols);
    const int coluna=(int)(pos%(size_t)cols);
    size_t inicio_linha=pagina_inicio+(size_t)linha*(size_t)cols;
    size_t restantes=total_visiveis>inicio_linha
        ? total_visiveis-inicio_linha : 0;
    int quantidade=restantes>(size_t)cols?cols:(int)restantes;
    int largura=quantidade*lado+(quantidade-1)*gap_x;
    return (SDL_Rect){
        area.x+(area.w-largura)/2+coluna*(lado+gap_x),
        y0+linha*(lado+gap_y),lado,lado
    };
}
int carousel_mover_grade_vertical(int direcao) {
    if(!total_visiveis || !direcao) return 0;
    /* O movimento percorre a mesma ordem da grade com 5 colunas. */
    size_t passo=(size_t)colunas_grade_atual;
    if(direcao<0) {
        if(foco_atual<passo) return 0;
        foco_atual-=passo;
    } else {
        if(foco_atual+passo>=total_visiveis) return 0;
        foco_atual+=passo;
    }
    return 1;
}
int carousel_clicar_grade(int x,int y,SDL_Rect area) {
    if(!total_visiveis || area.w<=0 || area.h<=0) return 0;
    int cols,lado,gx,gy,y0;
    grade_geometria(area,&cols,&lado,&gx,&gy,&y0);
    size_t tamanho_pagina=(size_t)cols*2;
    size_t inicio=(foco_atual/tamanho_pagina)*tamanho_pagina;
    size_t fim=inicio+tamanho_pagina;
    if(fim>total_visiveis) fim=total_visiveis;
    for(size_t i=inicio;i<fim;++i) {
        SDL_Rect r=grade_celula(area,cols,lado,gx,gy,y0,inicio,i-inicio);
        if(x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h) {
            if(i==foco_atual) return 2;
            foco_atual=i;
            return 1;
        }
    }
    return 0;
}
void desenhar_carousel(SDL_Rect area) {
    if (total_visiveis == 0 || area.w <= 0 || area.h <= 0) return;

    if (ui_game_view_get()==GAME_VIEW_CARD) {
        CardLayout layout=ui_card_layout_get();
        if(layout==CARD_LAYOUT_GRID) {
            int cols,lado,gx,gy,y0;
            grade_geometria(area,&cols,&lado,&gx,&gy,&y0);
            size_t tamanho_pagina=(size_t)cols*2;
            size_t inicio=(foco_atual/tamanho_pagina)*tamanho_pagina;
            size_t fim=inicio+tamanho_pagina;
            if(fim>total_visiveis) fim=total_visiveis;
            for(size_t i=inicio;i<fim;++i) {
                SDL_Rect pos=grade_celula(area,cols,lado,gx,gy,y0,
                                          inicio,i-inicio);
                desenhar_jogo(indices[i],pos,i==foco_atual?255:230);
            }
            return;
        }

        /* Cartuchos nao entram nestes modos: aqui so variam capas digitais. */
        int cinco=layout==CARD_LAYOUT_5 && total_visiveis>=5;
        int lado=(int)(area.h*(cinco?0.46f:0.52f));
        if(lado>245) lado=245;
        if(lado<64) lado=64;
        int vizinho,externo,gap,dist1,dist2;
        for(;;) {
            vizinho=lado*87/100;
            externo=lado*82/100;
            gap=lado*(cinco?18:22)/100;
            if(gap<8) gap=8;
            dist1=(lado+vizinho)/2+gap;
            dist2=dist1+(vizinho+externo)/2+gap;
            int alcance=cinco?2*dist2+externo:2*dist1+vizinho;
            if(alcance<=area.w-25 || lado<=64) break;
            lado-=2;
        }
        const int cx=area.x+area.w/2;
        const int cy=area.y+(int)(area.h*0.47f);
        if(cinco) {
            size_t e2=(foco_atual+total_visiveis-2)%total_visiveis;
            size_t d2=(foco_atual+2)%total_visiveis;
            desenhar_jogo(indices[e2],(SDL_Rect){cx-dist2-externo/2,
                        cy-externo/2,externo,externo},210);
            desenhar_jogo(indices[d2],(SDL_Rect){cx+dist2-externo/2,
                        cy-externo/2,externo,externo},210);
        }
        if(total_visiveis>=3) {
            size_t esq=(foco_atual+total_visiveis-1)%total_visiveis;
            size_t dir=(foco_atual+1)%total_visiveis;
            desenhar_jogo(indices[esq],(SDL_Rect){cx-dist1-vizinho/2,
                         cy-vizinho/2,vizinho,vizinho},235);
            desenhar_jogo(indices[dir],(SDL_Rect){cx+dist1-vizinho/2,
                         cy-vizinho/2,vizinho,vizinho},235);
        } else if(total_visiveis==2) {
            size_t outro=(foco_atual+1)%total_visiveis;
            desenhar_jogo(indices[outro],(SDL_Rect){cx+dist1-vizinho/2,
                         cy-vizinho/2,vizinho,vizinho},235);
        }
        desenhar_jogo(indices[foco_atual],(SDL_Rect){cx-lado/2,
                       cy-lado/2,lado,lado},255);
        return;
    }

    /*
     * O tamanho deixa de acompanhar toda a altura disponivel. O layout de
     * referencia usa cartuchos compactos; limitamos a caixa por altura e
     * largura para manter margens generosas em 16:9 e em janelas menores.
     */
    int cart_box = (int)(area.h * 0.62f);
    int limite_largura = (int)(area.w * 0.30f);
    if (cart_box > limite_largura) cart_box = limite_largura;
    if (cart_box > 340) cart_box = 340;
    if (cart_box < 64) cart_box = 64;

    /* Laterais menores, com respiro visual ao redor do cartucho em foco.
       Mantem o centro e o tamanho das artes; muda somente a distancia lateral. */
    const int side_box = cart_box * 76 / 100;
    const int side_offset = cart_box * 108 / 100;

    const int center_x = area.x + area.w / 2;
    const int center_y = area.y + (int)(area.h * 0.47f);
    const int main_x = center_x - cart_box / 2;
    const int main_y = center_y - cart_box / 2;

    if (total_visiveis > 1) {
        size_t esquerda = foco_atual == 0 ? total_visiveis - 1 : foco_atual - 1;
        size_t direita = foco_atual + 1 == total_visiveis ? 0 : foco_atual + 1;

        desenhar_jogo(indices[esquerda],
            (SDL_Rect){center_x - side_offset - side_box / 2,
                       center_y - side_box / 2, side_box, side_box}, 100);

        desenhar_jogo(indices[direita],
            (SDL_Rect){center_x + side_offset - side_box / 2,
                       center_y - side_box / 2, side_box, side_box}, 100);
    }

    /* Flutuacao sutil exclusiva do cartucho em destaque. */
    float tempo = SDL_GetTicks() / 300.0f;
    int flutuacao_y = (int)(sin(tempo) * 5.0f);
    desenhar_jogo(indices[foco_atual],
        (SDL_Rect){main_x, main_y + flutuacao_y, cart_box, cart_box}, 255);
}

void mover_carousel_esquerda(void) {
    if (total_visiveis > 0)
        foco_atual = foco_atual == 0 ? total_visiveis - 1 : foco_atual - 1;
}

void mover_carousel_direita(void) {
    if (total_visiveis > 0)
        foco_atual = (foco_atual + 1) % total_visiveis;
}

void fechar_carousel(void) {
    ui_cards_liberar_ajustes();
    for (size_t i = 0; i < total_consoles; ++i) {
        if (runtime_consoles && runtime_consoles[i].textura_molde)
            SDL_DestroyTexture(runtime_consoles[i].textura_molde);
    }

    for (size_t i = 0; i < total_jogos; ++i) {
        if (jogos[i].arte_capa)
            SDL_DestroyTexture(jogos[i].arte_capa);
        if (jogos[i].arte_card)
            SDL_DestroyTexture(jogos[i].arte_card);
    }

    free(runtime_consoles);
    free(jogos);
    free(indices);

    runtime_consoles = NULL;
    jogos = NULL;
    indices = NULL;
    console_filtro = NULL;
    total_consoles = total_jogos = capacidade_jogos = foco_atual = total_visiveis = 0;
}