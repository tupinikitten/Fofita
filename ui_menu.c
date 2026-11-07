#include "ui_menu.h"
#include "ui_text.h"
#include "ui_theme.h"
#include "ui_carousel.h"
#include "video_audio.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>

#define MAX_ITENS 32
#define MAX_OCULTOS 32
#define TAM_ID 32
#define ARQUIVO_CONFIG "fofita_menu.cfg"
#define DURACAO_AVISO_MS 3500

/* Layout (janela de 800x600): cabecalho, carrossel, linha do titulo, rodape. */
#define ALTURA_CABECALHO 96
#define ALTURA_TITULO 44
#define ALTURA_RODAPE 60
#define SLOT_W 104
#define SLOT_H 64
#define ICONE_MAX_W 72
#define ICONE_MAX_H 48
#define SETA_TAM 28
#define BOTAO_W 220
#define BOTAO_H 34
#define SIDEBAR_RECOLHIDA 68
#define SIDEBAR_ABERTA 184
#define SIDEBAR_ITEM_H 58
#define SIDEBAR_TOPO 104
#define SIDEBAR_ICON 28
#define TOTAL_ACOES_LATERAIS 6

typedef enum { FOCO_CABECALHO, FOCO_CARROSSEL, FOCO_SAIR, FOCO_SIDEBAR } Foco;
typedef enum {
    LATERAL_CONQUISTAS, LATERAL_CONFIGURACOES, LATERAL_FILTROS,
    LATERAL_FAVORITOS, LATERAL_APARENCIA, LATERAL_SAIR
} AcaoLateral;

typedef struct {
    const ConsoleConfig *cfg;
    SDL_Texture *icone;
    int icone_w, icone_h;
} ItemConsole;

/* So entram aqui os consoles ativos (com nucleo disponivel). */
static ItemConsole itens[MAX_ITENS];
static int total_itens = 0;

/* Consoles do cabecalho = ativos que o usuario nao ocultou. Guarda indices de 'itens'. */
static int visiveis[MAX_ITENS];
static int total_visiveis = 0;
static int sel = 0;   /* posicao dentro de 'visiveis' */

static char ocultos[MAX_OCULTOS][TAM_ID];
static int total_ocultos = 0;

static SDL_Texture *wallpaper = NULL;
static SDL_Texture *seta_esq = NULL;
static SDL_Texture *seta_dir = NULL;
static SDL_Texture *icones_laterais[TOTAL_ACOES_LATERAIS] = {0};
static const char *const caminhos_icones_laterais[TOTAL_ACOES_LATERAIS] = {
    "ui/styles/dark/icons/trophy_icon.png",
    "ui/styles/dark/icons/settings_icon.png",
    "ui/styles/dark/icons/filter_icon.png",
    "ui/styles/dark/icons/heart_icon.png",
    "ui/styles/dark/icons/brush_icon.png",
    "ui/styles/dark/icons/out_icon.png"
};
static const char *const nomes_laterais[TOTAL_ACOES_LATERAIS] = {
    "Conquistas", "Configuracoes", "Filtros", "Favoritos", "Aparencia", "Sair"
};
static int sidebar_aberta = 0;
static int item_lateral = LATERAL_CONFIGURACOES;
static int tela_configuracoes = 0;
static int opcao_configuracoes = 0;

static Foco foco = FOCO_CARROSSEL;
static char aviso[128] = "";
static Uint32 aviso_ate = 0;

/* ---------- configuracao (consoles ocultos) ---------- */

static int indice_oculto(const char *id) {
    for (int i = 0; i < total_ocultos; ++i)
        if (strcmp(ocultos[i], id) == 0) return i;
    return -1;
}

static int esta_oculto(const char *id) {
    return indice_oculto(id) >= 0;
}

static void definir_oculto(const char *id, int ocultar) {
    int i = indice_oculto(id);

    if (ocultar && i < 0 && total_ocultos < MAX_OCULTOS) {
        snprintf(ocultos[total_ocultos++], TAM_ID, "%s", id);
    } else if (!ocultar && i >= 0) {
        memmove(ocultos[i], ocultos[i + 1], (size_t)(total_ocultos - i - 1) * TAM_ID);
        total_ocultos--;
    }
}

static void carregar_config(void) {
    FILE *f = fopen(ARQUIVO_CONFIG, "r");
    if (!f) return;

    char linha[128];
    while (fgets(linha, sizeof(linha), f)) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (strncmp(linha, "oculto=", 7) == 0 && linha[7] != '\0')
            definir_oculto(linha + 7, 1);
    }

    fclose(f);
}

static void salvar_config(void) {
    FILE *f = fopen(ARQUIVO_CONFIG, "w");
    if (!f) return;

    for (int i = 0; i < total_ocultos; ++i)
        fprintf(f, "oculto=%s\n", ocultos[i]);

    fclose(f);
}

/* ---------- selecao de console ---------- */

static const ConsoleConfig *console_selecionado(void) {
    return total_visiveis > 0 ? itens[visiveis[sel]].cfg : NULL;
}

static void aplicar_selecao(void) {
    carousel_definir_console(console_selecionado());
}

static void reconstruir_visiveis(const ConsoleConfig *manter) {
    total_visiveis = 0;
    sel = 0;

    for (int i = 0; i < total_itens; ++i) {
        if (esta_oculto(itens[i].cfg->id)) continue;
        if (itens[i].cfg == manter) sel = total_visiveis;
        visiveis[total_visiveis++] = i;
    }
}

static void trocar_console(int direcao) {
    if (total_visiveis < 2) return;
    sel = (sel + direcao + total_visiveis) % total_visiveis;
    aplicar_selecao();
}

static void ocultar_console_atual(void) {
    if (total_visiveis < 2) {
        ui_menu_aviso("Nao da para ocultar o unico console");
        return;
    }

    definir_oculto(console_selecionado()->id, 1);
    salvar_config();

    /* O proximo console visivel assume a posicao do que saiu. */
    int antigo = sel;
    reconstruir_visiveis(NULL);
    sel = antigo < total_visiveis ? antigo : total_visiveis - 1;
    aplicar_selecao();
}

static void mostrar_todos_consoles(void) {
    if (total_ocultos == 0) return;

    const ConsoleConfig *atual = console_selecionado();
    total_ocultos = 0;
    salvar_config();
    reconstruir_visiveis(atual);
    aplicar_selecao();
    ui_menu_aviso("Todos os consoles voltaram ao menu");
}

/* ---------- inicializacao ---------- */

static SDL_Texture *carregar_textura(const char *caminho, int *w, int *h) {
    SDL_Texture *tex = IMG_LoadTexture(renderer, caminho);

    if (!tex) {
        printf("Aviso: imagem %s nao encontrada.\n", caminho);
        if (w) *w = 0;
        if (h) *h = 0;
        return NULL;
    }

    SDL_QueryTexture(tex, NULL, NULL, w, h);
    return tex;
}

int ui_menu_init(void) {
    const UiTema *tema = ui_tema_atual();
    char caminho[512];

    if (!ui_texto_init(renderer)) {
        fprintf(stderr, "Erro: nao foi possivel preparar a fonte do menu.\n");
        return 0;
    }

    wallpaper = carregar_textura(tema->wallpaper, NULL, NULL);

    snprintf(caminho, sizeof(caminho), "%s/arrows/left_arrow.png", tema->pasta_estilo);
    seta_esq = carregar_textura(caminho, NULL, NULL);
    snprintf(caminho, sizeof(caminho), "%s/arrows/right_arrow.png", tema->pasta_estilo);
    seta_dir = carregar_textura(caminho, NULL, NULL);

    for (int i = 0; i < TOTAL_ACOES_LATERAIS; ++i)
        icones_laterais[i] = carregar_textura(caminhos_icones_laterais[i], NULL, NULL);

    size_t quantidade = 0;
    const ConsoleConfig *todos = console_registry_get_all(&quantidade);

    for (size_t i = 0; i < quantidade && total_itens < MAX_ITENS; ++i) {
        if (!console_registry_esta_ativo(&todos[i])) continue;

        ItemConsole *item = &itens[total_itens++];
        memset(item, 0, sizeof(*item));
        item->cfg = &todos[i];

        if (todos[i].caminho_icone)
            item->icone = carregar_textura(todos[i].caminho_icone, &item->icone_w, &item->icone_h);
    }

    carregar_config();
    reconstruir_visiveis(NULL);
    aplicar_selecao();
    foco = FOCO_CARROSSEL;
    sidebar_aberta = 0;
    item_lateral = LATERAL_CONFIGURACOES;
    return 1;
}

void ui_menu_fechar(void) {
    for (int i = 0; i < total_itens; ++i)
        if (itens[i].icone) SDL_DestroyTexture(itens[i].icone);

    if (wallpaper) SDL_DestroyTexture(wallpaper);
    if (seta_esq) SDL_DestroyTexture(seta_esq);
    if (seta_dir) SDL_DestroyTexture(seta_dir);
    for (int i = 0; i < TOTAL_ACOES_LATERAIS; ++i)
        if (icones_laterais[i]) SDL_DestroyTexture(icones_laterais[i]);

    wallpaper = seta_esq = seta_dir = NULL;
    total_itens = total_visiveis = sel = 0;
    ui_texto_fechar();
}

/* ---------- entrada ---------- */

const ConsoleConfig *ui_menu_console_atual(void) {
    return console_selecionado();
}

void ui_menu_focar_carrossel(void) {
    foco = FOCO_CARROSSEL;
}

void ui_menu_aviso(const char *mensagem) {
    snprintf(aviso, sizeof(aviso), "%s", mensagem ? mensagem : "");
    aviso_ate = SDL_GetTicks() + DURACAO_AVISO_MS;
}

static int tecla_confirmar(SDL_Keycode t) {
    return t == SDLK_RETURN || t == SDLK_KP_ENTER || t == SDLK_x;
}

MenuAcao ui_menu_tecla(SDL_Keycode tecla, Uint16 modificadores) {
    if (tela_configuracoes) {
        if (tecla == SDLK_UP || tecla == SDLK_DOWN)
            opcao_configuracoes = 1 - opcao_configuracoes;
        else if (tecla == SDLK_ESCAPE || (tecla == SDLK_LEFT && opcao_configuracoes == 1))
            tela_configuracoes = 0;
        else if (tecla_confirmar(tecla)) {
            if (opcao_configuracoes == 0) {
                mostrar_todos_consoles();
            } else {
                tela_configuracoes = 0;
            }
        }
        return MENU_ACAO_NENHUMA;
    }

    if (tecla == SDLK_TAB) {
        sidebar_aberta = !sidebar_aberta;
        foco = FOCO_SIDEBAR;
        return MENU_ACAO_NENHUMA;
    }

    if (foco == FOCO_SIDEBAR) {
        if (tecla == SDLK_UP) {
            item_lateral = (item_lateral + TOTAL_ACOES_LATERAIS - 1) % TOTAL_ACOES_LATERAIS;
        } else if (tecla == SDLK_DOWN) {
            item_lateral = (item_lateral + 1) % TOTAL_ACOES_LATERAIS;
        } else if (tecla == SDLK_RIGHT || tecla == SDLK_ESCAPE) {
            foco = FOCO_CARROSSEL;
        } else if (tecla == SDLK_LEFT) {
            sidebar_aberta = !sidebar_aberta;
        } else if (tecla_confirmar(tecla)) {
            switch (item_lateral) {
            case LATERAL_SAIR:
                return MENU_ACAO_SAIR;
            case LATERAL_CONFIGURACOES:
                tela_configuracoes = 1;
                opcao_configuracoes = 0;
                break;
            case LATERAL_APARENCIA:
                ui_menu_aviso("Personalizacao: em desenvolvimento");
                break;
            case LATERAL_CONQUISTAS:
                ui_menu_aviso("RetroAchievements: em desenvolvimento");
                break;
            case LATERAL_FILTROS:
                ui_menu_aviso("Filtros: em desenvolvimento");
                break;
            case LATERAL_FAVORITOS:
                ui_menu_aviso("Favoritos: em desenvolvimento");
                break;
            }
        }
        return MENU_ACAO_NENHUMA;
    }

    switch (foco) {
    case FOCO_CABECALHO:
        if (tecla == SDLK_LEFT) trocar_console(-1);
        else if (tecla == SDLK_RIGHT) trocar_console(+1);
        else if (tecla == SDLK_DOWN || tecla_confirmar(tecla)) foco = FOCO_CARROSSEL;
        else if (tecla == SDLK_h) {
            if (modificadores & KMOD_SHIFT) mostrar_todos_consoles();
            else if (total_visiveis > 0) ocultar_console_atual();
        }
        break;

    case FOCO_CARROSSEL:
        if (tecla == SDLK_LEFT) mover_carousel_esquerda();
        else if (tecla == SDLK_RIGHT) mover_carousel_direita();
        else if (tecla == SDLK_UP) foco = FOCO_CABECALHO;
        else if (tecla == SDLK_DOWN) foco = FOCO_SAIR;
        else if (tecla_confirmar(tecla)) {
            if (total_visiveis > 0 && carousel_total_visiveis() > 0)
                return MENU_ACAO_INICIAR_JOGO;
            ui_menu_aviso("Nenhuma ROM neste console");
        }
        break;

    case FOCO_SAIR:
        if (tecla == SDLK_UP) foco = FOCO_CARROSSEL;
        else if (tecla == SDLK_LEFT) foco = FOCO_SIDEBAR;
        else if (tecla_confirmar(tecla)) return MENU_ACAO_SAIR;
        break;
    case FOCO_SIDEBAR:
        break;
    }

    return MENU_ACAO_NENHUMA;
}

/* ---------- desenho ---------- */

static void preencher(SDL_Rect r, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer, &r);
}

static void contorno(SDL_Rect r, int espessura, SDL_Color c) {
    preencher((SDL_Rect){ r.x, r.y, r.w, espessura }, c);
    preencher((SDL_Rect){ r.x, r.y + r.h - espessura, r.w, espessura }, c);
    preencher((SDL_Rect){ r.x, r.y + espessura, espessura, r.h - 2 * espessura }, c);
    preencher((SDL_Rect){ r.x + r.w - espessura, r.y + espessura, espessura, r.h - 2 * espessura }, c);
}

/* Papel de parede em modo "cobrir": preenche a janela cortando o excesso, sem deformar. */
static void desenhar_fundo(int vw, int vh) {
    const UiTema *tema = ui_tema_atual();

    if (!wallpaper) {
        preencher((SDL_Rect){ 0, 0, vw, vh }, tema->cor_fundo);
        return;
    }

    int tw = 0, th = 0;
    SDL_QueryTexture(wallpaper, NULL, NULL, &tw, &th);

    if (tw <= 0 || th <= 0) {
        SDL_RenderCopy(renderer, wallpaper, NULL, NULL);
        return;
    }

    float escala = (float)vw / tw;
    if ((float)vh / th > escala) escala = (float)vh / th;

    SDL_Rect origem;
    origem.w = (int)(vw / escala + 0.5f);
    origem.h = (int)(vh / escala + 0.5f);
    if (origem.w > tw) origem.w = tw;
    if (origem.h > th) origem.h = th;
    origem.x = (tw - origem.w) / 2;
    origem.y = (th - origem.h) / 2;

    SDL_RenderCopy(renderer, wallpaper, &origem, NULL);
}

/* Desenha 'tex' (w x h) encaixada e centralizada em 'caixa', sem deformar. */
static void desenhar_encaixado(SDL_Texture *tex, int w, int h, SDL_Rect caixa, Uint8 alfa) {
    if (!tex || w <= 0 || h <= 0) return;

    float escala = (float)caixa.w / w;
    if ((float)caixa.h / h < escala) escala = (float)caixa.h / h;

    int dw = (int)(w * escala + 0.5f);
    int dh = (int)(h * escala + 0.5f);
    SDL_Rect destino = { caixa.x + (caixa.w - dw) / 2, caixa.y + (caixa.h - dh) / 2, dw, dh };

    SDL_SetTextureAlphaMod(tex, alfa);
    SDL_RenderCopy(renderer, tex, NULL, &destino);
    SDL_SetTextureAlphaMod(tex, 255);
}

static void desenhar_cabecalho(int x_area, int largura_area) {
    int vw = largura_area;
    const UiTema *tema = ui_tema_atual();

    if (total_visiveis == 0) {
        ui_texto_desenhar_centro("Nenhum console ativo", x_area + vw / 2, 30, 2,
                                 tema->texto, tema->texto_sombra);
        ui_texto_desenhar_centro("Confira a pasta cores/", x_area + vw / 2, 52, 2,
                                 tema->texto_suave, tema->texto_sombra);
        return;
    }

    int max_slots = (vw - 40) / SLOT_W;
    if (max_slots < 1) max_slots = 1;

    int inicio = 0;
    if (total_visiveis > max_slots) {
        inicio = sel - max_slots / 2;
        if (inicio < 0) inicio = 0;
        if (inicio > total_visiveis - max_slots) inicio = total_visiveis - max_slots;
    }

    int mostrados = total_visiveis - inicio;
    if (mostrados > max_slots) mostrados = max_slots;

    int x0 = x_area + (vw - mostrados * SLOT_W) / 2;
    const int y0 = 6;

    for (int k = 0; k < mostrados; ++k) {
        int pos = inicio + k;
        const ItemConsole *item = &itens[visiveis[pos]];
        int selecionado = (pos == sel);
        SDL_Rect slot = { x0 + k * SLOT_W + 4, y0, SLOT_W - 8, SLOT_H };

        if (selecionado) {
            preencher(slot, tema->painel);
            contorno(slot, foco == FOCO_CABECALHO ? 3 : 1, tema->destaque);
        }

        SDL_Rect caixa_icone = { slot.x + (slot.w - ICONE_MAX_W) / 2,
                                 slot.y + (slot.h - ICONE_MAX_H) / 2,
                                 ICONE_MAX_W, ICONE_MAX_H };

        if (item->icone) {
            desenhar_encaixado(item->icone, item->icone_w, item->icone_h,
                               caixa_icone, selecionado ? 255 : 140);
        } else {
            /* Sem icone: mostra o identificador do console. */
            char sigla[TAM_ID];
            snprintf(sigla, sizeof(sigla), "%s", item->cfg->id);
            for (char *c = sigla; *c; ++c)
                if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
            ui_texto_desenhar_centro(sigla, slot.x + slot.w / 2,
                                     slot.y + (slot.h - ui_texto_altura(2)) / 2, 2,
                                     tema->texto, tema->texto_sombra);
        }
    }

    char nome[96];
    ui_texto_ajustar(console_selecionado()->nome, vw - 40, 2, nome, sizeof(nome));
    ui_texto_desenhar_centro(nome, x_area + vw / 2, y0 + SLOT_H + 6, 2, tema->texto, tema->texto_sombra);
}

static void desenhar_linha_titulo(int x_area, int vw, int y) {
    const UiTema *tema = ui_tema_atual();

    if (carousel_total_visiveis() == 0) return;

    char bruto[256], nome[256], contador[32];
    carousel_nome_jogo_atual(bruto, sizeof(bruto));

    int max_texto = vw - 2 * (SETA_TAM + 36);
    ui_texto_ajustar(bruto, max_texto, 2, nome, sizeof(nome));
    ui_texto_desenhar_centro(nome, x_area + vw / 2, y + 2, 2, tema->texto, tema->texto_sombra);

    snprintf(contador, sizeof(contador), "%zu / %zu",
             carousel_posicao_atual() + 1, carousel_total_visiveis());
    ui_texto_desenhar_centro(contador, x_area + vw / 2, y + 24, 2, tema->texto_suave, tema->texto_sombra);

    if (carousel_total_visiveis() > 1) {
        int meio = ui_texto_largura(nome, 2) / 2;
        Uint8 alfa = foco == FOCO_CARROSSEL ? 255 : 130;
        SDL_Rect esq = { x_area + vw / 2 - meio - 16 - SETA_TAM, y + 8, SETA_TAM, SETA_TAM };
        SDL_Rect dir = { x_area + vw / 2 + meio + 16, y + 8, SETA_TAM, SETA_TAM };

        desenhar_encaixado(seta_esq, 512, 512, esq, alfa);
        desenhar_encaixado(seta_dir, 512, 512, dir, alfa);
    }
}

static void desenhar_rodape(int x_area, int vw, int y) {
    const UiTema *tema = ui_tema_atual();
    SDL_Rect botao = { x_area + (vw - BOTAO_W) / 2, y + (ALTURA_RODAPE - BOTAO_H) / 2, BOTAO_W, BOTAO_H };
    int em_foco = (foco == FOCO_SAIR);

    if (em_foco) {
        preencher(botao, tema->destaque);
    } else {
        preencher(botao, tema->painel);
        contorno(botao, 1, tema->destaque);
    }

    SDL_Color cor_texto = em_foco ? (SDL_Color){ 255, 255, 255, 255 } : tema->texto;
    SDL_Color sem_sombra = { 0, 0, 0, 0 };

    ui_texto_desenhar_centro("Sair do Fofita", botao.x + botao.w / 2,
                             botao.y + (botao.h - ui_texto_altura(2)) / 2, 2,
                             cor_texto, em_foco ? sem_sombra : tema->texto_sombra);
}

static void desenhar_sidebar(int vh) {
    const UiTema *tema = ui_tema_atual();
    int largura = sidebar_aberta ? SIDEBAR_ABERTA : SIDEBAR_RECOLHIDA;
    SDL_Rect fundo = { 0, 0, largura, vh };
    preencher(fundo, (SDL_Color){ 18, 24, 40, 220 });
    preencher((SDL_Rect){ largura - 1, 0, 1, vh }, tema->destaque);

    for (int i = 0; i < TOTAL_ACOES_LATERAIS; ++i) {
        int y = SIDEBAR_TOPO + i * SIDEBAR_ITEM_H;
        SDL_Rect item = { 6, y, largura - 12, SIDEBAR_ITEM_H - 5 };
        int ativo = (foco == FOCO_SIDEBAR && item_lateral == i);
        if (ativo) preencher(item, tema->destaque);
        else if (i == LATERAL_SAIR) {
            /* Separacao visual para a acao de encerrar. */
            preencher((SDL_Rect){ 12, y - 8, largura - 24, 1 }, (SDL_Color){ 255,255,255,70 });
        }

        if (icones_laterais[i]) {
            SDL_Rect caixa = { 18, y + (SIDEBAR_ITEM_H - SIDEBAR_ICON - 5) / 2,
                               SIDEBAR_ICON, SIDEBAR_ICON };
            desenhar_encaixado(icones_laterais[i], SIDEBAR_ICON, SIDEBAR_ICON,
                               caixa, ativo ? 255 : 210);
        }
        if (sidebar_aberta) {
            SDL_Color cor = ativo ? (SDL_Color){255,255,255,255} : tema->texto;
            ui_texto_desenhar(nomes_laterais[i], 58, y + 20, 1, cor,
                              ativo ? (SDL_Color){0,0,0,0} : tema->texto_sombra);
        }
    }

    /* Indicador discreto de que Tab expande/recolhe a barra. */
    const char *dica = sidebar_aberta ? "TAB <" : "TAB >";
    ui_texto_desenhar_centro(dica, largura / 2, vh - 24, 1,
                             tema->texto_suave, (SDL_Color){0,0,0,0});
}

void ui_menu_desenhar(void) {
    const UiTema *tema = ui_tema_atual();
    int vw = 800, vh = 600;

    SDL_GetRendererOutputSize(renderer, &vw, &vh);
    if (vw <= 0) vw = 800;
    if (vh <= 0) vh = 600;

    desenhar_fundo(vw, vh);
    desenhar_sidebar(vh);

    int sidebar_w = sidebar_aberta ? SIDEBAR_ABERTA : SIDEBAR_RECOLHIDA;
    int x_area = sidebar_w;
    int largura_area = vw - sidebar_w;
    if (largura_area < 1) largura_area = 1;

    desenhar_cabecalho(x_area, largura_area);

    SDL_Rect area = { x_area, ALTURA_CABECALHO, largura_area,
                      vh - ALTURA_CABECALHO - ALTURA_TITULO - ALTURA_RODAPE };
    if (area.h < 0) area.h = 0;

    if (total_visiveis > 0) {
        if (carousel_total_visiveis() > 0) {
            desenhar_carousel(area);
        } else {
            char pasta[160];
            snprintf(pasta, sizeof(pasta), "Coloque as ROMs em %s/", console_selecionado()->pasta_roms);
            ui_texto_desenhar_centro("Nenhuma ROM encontrada", x_area + largura_area / 2,
                                     area.y + area.h / 2 - 20, 2,
                                     tema->texto, tema->texto_sombra);
            ui_texto_desenhar_centro(pasta, x_area + largura_area / 2,
                                     area.y + area.h / 2 + 4, 2,
                                     tema->texto_suave, tema->texto_sombra);
        }
    }

    int titulo_y = area.y + area.h;
    desenhar_linha_titulo(x_area, largura_area, titulo_y);
    desenhar_rodape(x_area, largura_area, titulo_y + ALTURA_TITULO);

    if (aviso[0] && SDL_GetTicks() < aviso_ate) {
        int larg = ui_texto_largura(aviso, 2) + 24;
        SDL_Rect faixa = { x_area + (largura_area - larg) / 2, area.y + 4,
                           larg, ui_texto_altura(2) + 12 };
        preencher(faixa, tema->painel);
        contorno(faixa, 1, tema->aviso);
        ui_texto_desenhar_centro(aviso, x_area + largura_area / 2, faixa.y + 6, 2,
                                 tema->aviso, (SDL_Color){ 0, 0, 0, 0 });
    }

    if (tela_configuracoes) {
        SDL_Rect painel = { x_area + (largura_area - 420) / 2, vh / 2 - 112, 420, 224 };
        preencher(painel, (SDL_Color){ 18, 24, 40, 245 });
        contorno(painel, 2, tema->destaque);
        ui_texto_desenhar_centro("CONFIGURACOES", painel.x + painel.w / 2,
                                 painel.y + 24, 2, tema->texto,
                                 (SDL_Color){0,0,0,0});
        SDL_Rect op1 = { painel.x + 24, painel.y + 82, painel.w - 48, 42 };
        SDL_Rect op2 = { painel.x + 24, painel.y + 136, painel.w - 48, 42 };
        if (opcao_configuracoes == 0) preencher(op1, tema->destaque);
        else preencher(op1, tema->painel);
        if (opcao_configuracoes == 1) preencher(op2, tema->destaque);
        else preencher(op2, tema->painel);
        SDL_Color c1 = opcao_configuracoes == 0 ? (SDL_Color){255,255,255,255} : tema->texto;
        SDL_Color c2 = opcao_configuracoes == 1 ? (SDL_Color){255,255,255,255} : tema->texto;
        ui_texto_desenhar_centro("Restaurar consoles ocultos", op1.x + op1.w / 2,
                                 op1.y + 15, 1, c1, (SDL_Color){0,0,0,0});
        ui_texto_desenhar_centro("Voltar ao menu", op2.x + op2.w / 2,
                                 op2.y + 15, 1, c2, (SDL_Color){0,0,0,0});
    }
}
