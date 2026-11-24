#include "ui_menu.h"
#include "ui_text.h"
#include "ui_theme.h"
#include "ui_carousel.h"
#include "ui_game_view.h"
#include "ui_cards.h"
#include "video_audio.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <stdlib.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_ITENS 32
#define MAX_OCULTOS 32
#define TAM_ID 32
#define ARQUIVO_CONFIG "fofita_menu.cfg"
#define DURACAO_AVISO_MS 3500

/* Layout responsivo: faixa superior compacta, carrossel e area livre inferior. */
#define ALTURA_CABECALHO 92
#define ALTURA_TITULO 48
#define AREA_ANIMACAO_FUTURA 104
#define SLOT_W 112
#define SLOT_H 58
#define ICONE_MAX_W 84
#define ICONE_MAX_H 54
#define SETA_TAM 24
#define SIDEBAR_RECOLHIDA 78
#define SIDEBAR_ITEM_H 54
#define SIDEBAR_ICON 29
#define TOTAL_ACOES_LATERAIS 6

typedef enum { FOCO_CABECALHO, FOCO_CARROSSEL, FOCO_SAIR, FOCO_SIDEBAR } Foco;
typedef enum {
    LATERAL_CONQUISTAS, LATERAL_CONFIGURACOES, LATERAL_FILTROS,
    LATERAL_FAVORITOS, LATERAL_CUSTOMIZACAO, LATERAL_SAIR
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
/* Apenas um mapa pequeno de brilho: nao manter a imagem completa na RAM. */
#define MAPA_BRILHO_W 96
#define MAPA_BRILHO_H 54
static Uint8 mapa_brilho[MAPA_BRILHO_W * MAPA_BRILHO_H];
static int mapa_brilho_valido = 0;
static SDL_Texture *seta_esq = NULL;
static SDL_Texture *seta_dir = NULL;
static SDL_Texture *icones_laterais[TOTAL_ACOES_LATERAIS] = {0};
static const char *const nomes_arquivos[TOTAL_ACOES_LATERAIS] = {
    "trophy", "settings", "filter", "heart", "bucket", "out"
};
/* Light = icones escuros; Dark = claros, Colorful = cores individuais. */
typedef enum { PRESET_LIGHT, PRESET_DARK, PRESET_COLORFUL } Preset;
static const char *const nomes_presets[] = {"Light", "Dark", "Colorful"};
static Preset manual_preset = PRESET_LIGHT;
static Preset auto_preset = PRESET_LIGHT;
static int contraste_automatico = 1;
/* Controle independente dos presets de icones da sidebar. */
typedef enum { TEXTO_AUTOMATICO, TEXTO_LIGHT, TEXTO_DARK } ModoTexto;
static ModoTexto modo_texto = TEXTO_AUTOMATICO;
static const char *const nomes_modos_texto[] = {"Automatico", "Light", "Dark"};
static char wallpaper_path[512] = "ui/wallpapers/bk-01.jpg";
#define MAX_WALLPAPERS 64
static char wallpapers[MAX_WALLPAPERS][512];
static int total_wallpapers = 0;
static int wallpaper_indice = 0;
static int tela_aparencia = 0;
static int opcao_aparencia = 0;
/* Submenu, sem adicionar botoes fixos a sidebar aprovada. */
static int tela_estilo_lateral = 0;
static int opcao_estilo_lateral = 0;
static int sidebar_pilula = 1;
static int consoles_cabecalho_visivel = 1;
#define MAX_PACOTES_USUARIO 32
#define TAM_PACOTE 48
static char pacotes_usuario[MAX_PACOTES_USUARIO][TAM_PACOTE];
static int total_pacotes_usuario = 0;
static char pacote_usuario_selecionado[TAM_PACOTE] = "";
/* Aparencia dos icones dos consoles, independente do pacote da sidebar. */
static int tela_estilo_consoles = 0;
static int opcao_estilo_consoles = 0;
static char pacote_consoles[TAM_PACOTE] = "";
static char pacotes_consoles[MAX_PACOTES_USUARIO][TAM_PACOTE];
static int total_pacotes_consoles = 0;
#define TOTAL_OPCOES_APARENCIA 10
#define ALTURA_MODAL_APARENCIA 583
static int tela_previa_wallpaper = 0;
static int preview_indice = 0;
static SDL_Texture *preview_textura = NULL;
static char preview_custom_path[1024] = "";

static int item_lateral = LATERAL_CONFIGURACOES;
static int tela_configuracoes = 0;
static int opcao_configuracoes = 0;
static int total_opcoes_config = 2;

static Foco foco = FOCO_CARROSSEL;
/* Visibilidade temporaria da sidebar: nao afeta a geometria do carrossel. */
static int sidebar_visivel = 1;
static char aviso[128] = "";
static Uint32 aviso_ate = 0;
static void limpar_cache_arredondado(void);

/* Menu unico, compacto e translucido. Mesmo retangulo controla
   renderizacao, clique e contorno da selecao. */
static void obter_geometria_sidebar(int vh, SDL_Rect *painel, int *lista_y,
                                   int *h_item, int *aberta) {
    if(aberta) *aberta=0;  /* parametro mantido para compatibilidade interna */
    int h=420;
    if(h>vh-28) h=vh-28;
    if(h<180) h=180;
    painel->x=12;
    painel->w=62;
    painel->h=h;
    painel->y=(vh-h)/2;
    int passo=(h-42)/TOTAL_ACOES_LATERAIS;
    if(passo>57) passo=57;
    if(passo<29) passo=29;
    if(h_item) *h_item=passo;
    if(lista_y) *lista_y=painel->y+(h-passo*TOTAL_ACOES_LATERAIS)/2;
}

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
        else if (strncmp(linha, "wallpaper=", 10)==0 && linha[10])
            snprintf(wallpaper_path,sizeof(wallpaper_path),"%s",linha+10);
        else if (strncmp(linha,"icon_auto=",10)==0)
            contraste_automatico = atoi(linha+10)!=0;
        else if (strncmp(linha,"icon_manual=",12)==0) {
            int v=atoi(linha+12);
            if(v>=0 && v<=2) manual_preset=(Preset)v;
        } else if (strncmp(linha,"text_contrast=",14)==0) {
            const char *valor = linha + 14;
            if (strcmp(valor,"light")==0) modo_texto=TEXTO_LIGHT;
            else if (strcmp(valor,"dark")==0) modo_texto=TEXTO_DARK;
            else modo_texto=TEXTO_AUTOMATICO;
        } else if (strncmp(linha,"sidebar_pack=",13)==0) {
            snprintf(pacote_usuario_selecionado,
                     sizeof(pacote_usuario_selecionado),"%s",linha+13);
        } else if (strncmp(linha,"sidebar_pill=",13)==0) {
            sidebar_pilula=atoi(linha+13)!=0;
        } else if (strncmp(linha,"console_strip=",14)==0) {
            consoles_cabecalho_visivel=atoi(linha+14)!=0;
        } else if (strncmp(linha,"console_icon_pack=",18)==0) {
            snprintf(pacote_consoles,sizeof(pacote_consoles),
                     "%s",linha+18);
        } else if (strncmp(linha,"card_layout=",12)==0) {
            const char *valor=linha+12;
            if(!strcmp(valor,"five")) ui_card_layout_set(CARD_LAYOUT_5);
            else if(!strcmp(valor,"grid")) ui_card_layout_set(CARD_LAYOUT_GRID);
            else ui_card_layout_set(CARD_LAYOUT_3);
        } else if (strncmp(linha,"game_view=",10)==0) {
            ui_game_view_set(strcmp(linha+10,"card")==0
                             ? GAME_VIEW_CARD : GAME_VIEW_CARTRIDGE);
        }
    }

    fclose(f);
}

static void salvar_config(void) {
    FILE *f = fopen(ARQUIVO_CONFIG, "w");
    if (!f) return;

    for (int i = 0; i < total_ocultos; ++i)
        fprintf(f, "oculto=%s\n", ocultos[i]);
    fprintf(f,"wallpaper=%s\nicon_auto=%d\nicon_manual=%d\n",
            wallpaper_path,contraste_automatico,(int)manual_preset);
    fprintf(f,"game_view=%s\n",ui_game_view_get()==GAME_VIEW_CARD?"card":"cartridge");
    fprintf(f,"card_layout=%s\n",
            ui_card_layout_get()==CARD_LAYOUT_GRID?"grid":
            ui_card_layout_get()==CARD_LAYOUT_5?"five":"three");
    fprintf(f,"text_contrast=%s\n",
            modo_texto==TEXTO_LIGHT?"light":modo_texto==TEXTO_DARK?"dark":"auto");
    fprintf(f,"sidebar_pack=%s\nsidebar_pill=%d\nconsole_strip=%d\n",
            pacote_usuario_selecionado,sidebar_pilula,
            consoles_cabecalho_visivel);
    fprintf(f,"console_icon_pack=%s\n",pacote_consoles);

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
    if(!consoles_cabecalho_visivel) {
        const ConsoleConfig *c=console_selecionado();
        if(c) ui_menu_aviso(c->nome);
    }
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

/* Os presets compartilham a mesma logica e os mesmos nomes-base. */
static Preset preset_ativo(void) { return contraste_automatico ? auto_preset : manual_preset; }
static SDL_Color icone_cor(int indice) {
    static const SDL_Color cores[6]={{244,192,89,255},{123,176,249,255},
        {104,215,189,255},{248,128,157,255},{192,144,246,255},{248,168,116,255}};
    Preset p=preset_ativo();
    if(p==PRESET_COLORFUL) return cores[indice];
    /* Light = icones claros sobre wallpaper escuro; Dark = icones escuros. */
    return p==PRESET_LIGHT ? (SDL_Color){248,249,255,255}
                           : (SDL_Color){33,45,66,255};
}
/* Cada preset usa imagens canonicas com sufixo de tema.
   Pre-filtragem por media de area para os PNGs 512x512:
   reduzimos para ate 64px no carregamento, preservando a silhueta e
   a transparencia antes da reducao final (~64 -> 29) com filtro linear.
   Executado somente ao trocar o preset, nunca a cada frame. */
static SDL_Texture *icone_textura_suavizada(const char *caminho,
                                          const SDL_Color *tingir) {
    SDL_Surface *orig=IMG_Load(caminho);
    if(!orig) return NULL;
    SDL_Surface *src=SDL_ConvertSurfaceFormat(orig,SDL_PIXELFORMAT_RGBA32,0);
    SDL_FreeSurface(orig);
    if(!src) return NULL;

    /* Area-average 512->64 preserva tracos finos melhor do que 512->128->29. */
    int tw=src->w>64?64:src->w;
    int th=src->h>64?64:src->h;
    if(tw<=0 || th<=0) { SDL_FreeSurface(src); return NULL; }
    SDL_Surface *dst=SDL_CreateRGBSurfaceWithFormat(0,tw,th,32,SDL_PIXELFORMAT_RGBA32);
    if(!dst) { SDL_FreeSurface(src); return NULL; }
    if(SDL_MUSTLOCK(src) && SDL_LockSurface(src)!=0) {
        SDL_FreeSurface(dst); SDL_FreeSurface(src); return NULL;
    }
    if(SDL_MUSTLOCK(dst) && SDL_LockSurface(dst)!=0) {
        if(SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
        SDL_FreeSurface(dst); SDL_FreeSurface(src); return NULL;
    }

    for(int y=0;y<th;++y) {
        int sy0=y*src->h/th,sy1=(y+1)*src->h/th;
        if(sy1<=sy0) sy1=sy0+1;
        Uint32 *linha=(Uint32*)((Uint8*)dst->pixels+y*dst->pitch);
        for(int x=0;x<tw;++x) {
            int sx0=x*src->w/tw,sx1=(x+1)*src->w/tw;
            if(sx1<=sx0) sx1=sx0+1;
            unsigned long long alpha=0,ra=0,ga=0,ba=0;
            unsigned long long count=(unsigned long long)(sx1-sx0)*(sy1-sy0);
            for(int sy=sy0;sy<sy1;++sy) {
                Uint32 *pixels=(Uint32*)((Uint8*)src->pixels+sy*src->pitch);
                for(int sx=sx0;sx<sx1;++sx) {
                    Uint8 rr,gg,bb,aa;
                    SDL_GetRGBA(pixels[sx],src->format,&rr,&gg,&bb,&aa);
                    alpha+=aa;
                    if(!tingir) {
                        ra+=(unsigned long long)rr*aa;
                        ga+=(unsigned long long)gg*aa;
                        ba+=(unsigned long long)bb*aa;
                    }
                }
            }
            /* Mesmo filtro para todos os icones; nenhum ganha espessura artificial. */
            Uint8 aa=(Uint8)((alpha+count/2)/count);
            /* Remove apenas ruido abaixo do limiar de visibilidade. */
            if(aa<=3) aa=0;
            Uint8 rr=0,gg=0,bb=0;
            if(tingir) {
                rr=tingir->r; gg=tingir->g; bb=tingir->b;
            } else if(alpha) {
                rr=(Uint8)((ra+alpha/2)/alpha);
                gg=(Uint8)((ga+alpha/2)/alpha);
                bb=(Uint8)((ba+alpha/2)/alpha);
            }
            linha[x]=SDL_MapRGBA(dst->format,rr,gg,bb,aa);
        }
    }

    if(SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
    if(SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
    SDL_Texture *tex=SDL_CreateTextureFromSurface(renderer,dst);
    SDL_FreeSurface(dst);
    SDL_FreeSurface(src);
    if(tex) {
        SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
        SDL_SetTextureScaleMode(tex,SDL_ScaleModeLinear);
#endif
    }
    return tex;
}
static SDL_Texture *carregar_icone_tingido(const char *caminho,SDL_Color cor) {
    return icone_textura_suavizada(caminho,&cor);
}
/* Nomes simples, sem barras nem traversal, para selecionar pacotes locais. */
static int nome_pacote_valido(const char *nome) {
    if(!nome || !*nome) return 0;
    size_t n=strlen(nome);
    if(n>32 || n>=TAM_PACOTE) return 0;
    if(!strcmp(nome,"light") || !strcmp(nome,"dark") ||
       !strcmp(nome,"colorful")) return 0;
    for(size_t i=0;i<n;++i) {
        unsigned char c=(unsigned char)nome[i];
        if(!((c>='A'&&c<='Z') || (c>='a'&&c<='z') ||
             (c>='0'&&c<='9') || c=='_' || c=='-')) return 0;
    }
    return 1;
}
static int ordenar_pacotes(const void *a,const void *b) {
    return strcmp((const char *)a,(const char *)b);
}
static int pacote_completo(const char *nome) {
    for(int i=0;i<TOTAL_ACOES_LATERAIS;++i) {
        char caminho[256];
        int n=snprintf(caminho,sizeof(caminho),
                "ui/styles/%s/icons/%s_icon_%s.png",
                nome,nomes_arquivos[i],nome);
        if(n<=0 || (size_t)n>=sizeof(caminho)) return 0;
        FILE *f=fopen(caminho,"rb");
        if(!f) return 0;
        fclose(f);
    }
    return 1;
}
static void escanear_pacotes_usuario(void) {
    total_pacotes_usuario=0;
    DIR *dir=opendir("ui/styles");
    if(dir) {
        struct dirent *entrada;
        while((entrada=readdir(dir))!=NULL &&
              total_pacotes_usuario<MAX_PACOTES_USUARIO) {
            if(!nome_pacote_valido(entrada->d_name) ||
               !pacote_completo(entrada->d_name)) continue;
            snprintf(pacotes_usuario[total_pacotes_usuario++],
                     TAM_PACOTE,"%s",entrada->d_name);
        }
        closedir(dir);
    }
    qsort(pacotes_usuario,(size_t)total_pacotes_usuario,
          sizeof(pacotes_usuario[0]),ordenar_pacotes);
    if(pacote_usuario_selecionado[0]) {
        int existe=0;
        for(int i=0;i<total_pacotes_usuario;++i)
            if(!strcmp(pacote_usuario_selecionado,pacotes_usuario[i]))
                existe=1;
        if(!existe) pacote_usuario_selecionado[0]='\0';
    }
}
static void proximo_pacote_usuario(void) {
    int atual=0; /* 0=padrao, 1..N=pacotes personalizados */
    for(int i=0;i<total_pacotes_usuario;++i)
        if(!strcmp(pacote_usuario_selecionado,pacotes_usuario[i]))
            atual=i+1;
    atual=(atual+1)%(total_pacotes_usuario+1);
    if(atual==0) pacote_usuario_selecionado[0]='\0';
    else snprintf(pacote_usuario_selecionado,
                  sizeof(pacote_usuario_selecionado),"%s",
                  pacotes_usuario[atual-1]);
}
/* Pacotes pessoais preservam as cores RGB de seus proprios PNGs.
   Os tres presets canonicos continuam com seu carregamento original. */
static void carregar_icones_preset(void) {
    const Preset preset=preset_ativo();
    const char *pasta=preset==PRESET_LIGHT?"light":
                      preset==PRESET_DARK?"dark":"colorful";
    const int custom=pacote_usuario_selecionado[0]!='\0';
    for(int i=0;i<TOTAL_ACOES_LATERAIS;++i) {
        if(icones_laterais[i]) {
            SDL_DestroyTexture(icones_laterais[i]);
            icones_laterais[i]=NULL;
        }
        char caminho[256];
        if(custom) {
            snprintf(caminho,sizeof(caminho),
                     "ui/styles/%s/icons/%s_icon_%s.png",
                     pacote_usuario_selecionado,nomes_arquivos[i],
                     pacote_usuario_selecionado);
            icones_laterais[i]=icone_textura_suavizada(caminho,NULL);
        }
        if(!icones_laterais[i]) {
            snprintf(caminho,sizeof(caminho),
                     "ui/styles/%s/icons/%s_icon_%s.png",
                     pasta,nomes_arquivos[i],pasta);
            if(preset==PRESET_COLORFUL)
                icones_laterais[i]=icone_textura_suavizada(caminho,NULL);
            else
                icones_laterais[i]=carregar_icone_tingido(caminho,icone_cor(i));
        }
        if(!icones_laterais[i])
            fprintf(stderr,"Fofita: icone de estilo ausente: %s\n",caminho);
    }
}
/* As setas seguem o mesmo contraste dos icones. Em Colorful,
   usamos o par claro/escuro conforme o wallpaper; nao existe pasta
   colorful/arrows e nao recorremos a icones de outro preset. */
static void carregar_setas_preset(void) {
    const Preset p=preset_ativo();
    const char *estilo=p==PRESET_COLORFUL
        ? (auto_preset==PRESET_LIGHT?"light":"dark")
        : (p==PRESET_LIGHT?"light":"dark");
    char caminho[256];
    snprintf(caminho,sizeof(caminho),"ui/styles/%s/arrows/left_arrow.png",estilo);
    SDL_Texture *esq=carregar_textura(caminho,NULL,NULL);
    snprintf(caminho,sizeof(caminho),"ui/styles/%s/arrows/right_arrow.png",estilo);
    SDL_Texture *dir=carregar_textura(caminho,NULL,NULL);
    if(esq) {
        if(seta_esq) SDL_DestroyTexture(seta_esq);
        seta_esq=esq;
    }
    if(dir) {
        if(seta_dir) SDL_DestroyTexture(seta_dir);
        seta_dir=dir;
    }
}
/* Cores de texto coerentes com o preset, sem alterar wallpaper. */
static SDL_Color cor_texto_ui(void) {
    return preset_ativo()!=PRESET_LIGHT ? (SDL_Color){32,45,68,255}
                                       : (SDL_Color){245,248,255,255};
}
static SDL_Color cor_texto_suave_ui(void) {
    return preset_ativo()!=PRESET_LIGHT ? (SDL_Color){62,78,120,255}
                                       : (SDL_Color){176,190,225,255};
}
static SDL_Color cor_sombra_ui(void) {
    return preset_ativo()!=PRESET_LIGHT ? (SDL_Color){255,255,255,170}
                                       : (SDL_Color){0,0,0,120};
}
/* O contraste de cada legenda usa a regiao em que ela aparece,
   independentemente do preset da sidebar, que observa o lado esquerdo. */
static SDL_Color cor_legenda_local(SDL_Rect regiao, int suave, SDL_Color *sombra) {
    if (modo_texto != TEXTO_AUTOMATICO) {
        int claro = modo_texto == TEXTO_LIGHT;
        if (sombra)
            *sombra = claro ? (SDL_Color){9,14,24,205}
                            : (SDL_Color){0,0,0,0};
        return claro
            ? (suave ? (SDL_Color){213,223,240,255}
                     : (SDL_Color){248,249,255,255})
            : (suave ? (SDL_Color){58,72,94,255}
                     : (SDL_Color){25,35,56,255});
    }
    int vw = 0, vh = 0;
    obter_tamanho_ui(&vw, &vh);
    if (!mapa_brilho_valido || vw <= 0 || vh <= 0) {
        /* Em fallback, nao usar contorno branco em tipografia escura. */
        if (sombra) *sombra = preset_ativo()==PRESET_LIGHT
            ? (SDL_Color){9,14,24,170} : (SDL_Color){0,0,0,0};
        return suave ? cor_texto_suave_ui() : cor_texto_ui();
    }

    int soma = 0;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 11; ++i) {
            int x = regiao.x + regiao.w * (2 * i + 1) / 22;
            int y = regiao.y + regiao.h * (2 * j + 1) / 6;
            if (x < 0) x = 0;
            if (x >= vw) x = vw - 1;
            if (y < 0) y = 0;
            if (y >= vh) y = vh - 1;
            soma += mapa_brilho[(y * MAPA_BRILHO_H / vh) * MAPA_BRILHO_W
                              + (x * MAPA_BRILHO_W / vw)];
        }
    }
    const int fundo_escuro = soma < 33 * 150;
    if (sombra)
        *sombra = fundo_escuro ? (SDL_Color){9,14,24,205}
                              : (SDL_Color){0,0,0,0};
    if (fundo_escuro)
        return suave ? (SDL_Color){213,223,240,255}
                     : (SDL_Color){248,249,255,255};
    return suave ? (SDL_Color){58,72,94,255}
                 : (SDL_Color){25,35,56,255};
}
static int extensao_imagem(const char *nome) {
    const char *p=strrchr(nome,'.');
    return p && (!SDL_strcasecmp(p,".jpg") || !SDL_strcasecmp(p,".jpeg") || !SDL_strcasecmp(p,".png"));
}
static void listar_wallpapers(void) {
    total_wallpapers=0;
    DIR *d=opendir("ui/wallpapers");
    if(d) {
        struct dirent *e;
        while((e=readdir(d))!=NULL && total_wallpapers<MAX_WALLPAPERS) {
            if(!extensao_imagem(e->d_name)) continue;
            snprintf(wallpapers[total_wallpapers++],512,"ui/wallpapers/%s",e->d_name);
        }
        closedir(d);
    }
    wallpaper_indice=0;
    for(int i=0;i<total_wallpapers;++i)
        if(strcmp(wallpaper_path,wallpapers[i])==0) wallpaper_indice=i;
}
static void avaliar_contraste_wallpaper(void) {
    mapa_brilho_valido = 0;
    SDL_Surface *s=IMG_Load(wallpaper_path);
    if(!s) { auto_preset=PRESET_DARK; return; }
    SDL_Surface *rgba=SDL_ConvertSurfaceFormat(s,SDL_PIXELFORMAT_RGBA32,0);
    SDL_FreeSurface(s);
    if(!rgba) { auto_preset=PRESET_DARK; return; }
    int w=rgba->w,h=rgba->h; double soma=0; int n=0;
    if(SDL_LockSurface(rgba)==0) {
        /* A amostra prioriza os 20% esquerdos, atras da pilula. */
        int lim=w/5; if(lim<1) lim=1;
        for(int y=h/8;y<h*7/8;y+=8) for(int x=0;x<lim;x+=8) {
            Uint8 *pix=(Uint8*)rgba->pixels+y*rgba->pitch+x*4;
            Uint8 r,g,b,a;
            Uint32 px; memcpy(&px,pix,4);
            SDL_GetRGBA(px,rgba->format,&r,&g,&b,&a);
            soma+=0.2126*r+0.7152*g+0.0722*b;
            ++n;
        }
        /* Replica o recorte central de desenhar_fundo() em 1280x720;
           esta amostragem so acontece quando o wallpaper e trocado. */
        int vw = 0, vh = 0;
        obter_tamanho_ui(&vw, &vh);
        if (vw > 0 && vh > 0 && w > 0 && h > 0) {
            float escala = fmaxf((float)vw / w, (float)vh / h);
            int origem_w = (int)(vw / escala + 0.5f);
            int origem_h = (int)(vh / escala + 0.5f);
            if (origem_w > w) origem_w = w;
            if (origem_h > h) origem_h = h;
            int origem_x = (w - origem_w) / 2;
            int origem_y = (h - origem_h) / 2;
            for (int my = 0; my < MAPA_BRILHO_H; ++my) {
                int sy = origem_y + origem_h * (2 * my + 1) / (2 * MAPA_BRILHO_H);
                if (sy >= h) sy = h - 1;
                for (int mx = 0; mx < MAPA_BRILHO_W; ++mx) {
                    int sx = origem_x + origem_w * (2 * mx + 1) / (2 * MAPA_BRILHO_W);
                    if (sx >= w) sx = w - 1;
                    Uint32 pixel;
                    memcpy(&pixel, (Uint8 *)rgba->pixels + sy * rgba->pitch + sx * 4, 4);
                    Uint8 r,g,b,a;
                    SDL_GetRGBA(pixel, rgba->format, &r, &g, &b, &a);
                    int brilho = (2126 * r + 7152 * g + 722 * b) / 10000;
                    /* Areas transparentes sao exibidas sobre o fundo escuro. */
                    mapa_brilho[my * MAPA_BRILHO_W + mx] = (Uint8)(brilho * a / 255);
                }
            }
            mapa_brilho_valido = 1;
        }
        SDL_UnlockSurface(rgba);
    }
    SDL_FreeSurface(rgba);
    if(!n) {auto_preset=PRESET_DARK;return;}
    double lum=soma/n;
    /* Histerese evita oscilacao em fundos proximo do limiar. */
    /* Papel escuro -> icones claros (Light). Papel claro -> icones escuros (Dark). */
    if(lum>155) auto_preset=PRESET_DARK;
    else if(lum<122) auto_preset=PRESET_LIGHT;
}
static void atualizar_wallpaper(const char *caminho,int persistir) {
    SDL_Texture *novo=IMG_LoadTexture(renderer,caminho);
    if(!novo) {ui_menu_aviso("Nao foi possivel abrir a imagem");return;}
    if(wallpaper) SDL_DestroyTexture(wallpaper);
    wallpaper=novo;
    if(caminho!=wallpaper_path) snprintf(wallpaper_path,sizeof(wallpaper_path),"%s",caminho);
    avaliar_contraste_wallpaper();
    carregar_icones_preset();
    carregar_setas_preset();
    if(persistir) salvar_config();
}
int ui_menu_init(void) {
    if (!ui_texto_init(renderer)) {
        fprintf(stderr, "Erro: nao foi possivel preparar a fonte do menu.\n");
        return 0;
    }

    carregar_config();
    escanear_pacotes_usuario();
    listar_wallpapers();
    atualizar_wallpaper(wallpaper_path,0);

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

    reconstruir_visiveis(NULL);
    aplicar_selecao();
    foco = FOCO_CARROSSEL;
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

    if(preview_textura) SDL_DestroyTexture(preview_textura);
    preview_textura=NULL;
    wallpaper = seta_esq = seta_dir = NULL;
    total_itens = total_visiveis = sel = 0;
    limpar_cache_arredondado();
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
    /* Edicao por ROM: bloquear troca de jogo/console ate salvar ou cancelar. */
    if (ui_cards_editando()) {
        if (tecla==SDLK_ESCAPE) {
            ui_cards_edicao_cancelar();
        } else if (tecla_confirmar(tecla)) {
            if (ui_cards_edicao_salvar()) {
                ui_som_tocar(UI_SOM_TROCA);
                ui_menu_aviso("Enquadramento salvo para este jogo");
            } else ui_menu_aviso("Nao foi possivel salvar fofita_cards.cfg");
        } else if (tecla==SDLK_LEFT) ui_cards_edicao_mover(-8,0);
        else if (tecla==SDLK_RIGHT) ui_cards_edicao_mover(8,0);
        else if (tecla==SDLK_UP) ui_cards_edicao_mover(0,-8);
        else if (tecla==SDLK_DOWN) ui_cards_edicao_mover(0,8);
        else if (tecla==SDLK_PLUS || tecla==SDLK_EQUALS ||
                 tecla==SDLK_KP_PLUS) ui_cards_edicao_zoom(5);
        else if (tecla==SDLK_MINUS || tecla==SDLK_KP_MINUS)
            ui_cards_edicao_zoom(-5);
        else if (tecla==SDLK_f) {
            ui_cards_edicao_alternar_modo();
            ui_som_tocar(UI_SOM_TROCA);
        } else if (tecla==SDLK_r) ui_cards_edicao_restaurar();
        return MENU_ACAO_NENHUMA;
    }
    if (tela_previa_wallpaper) {
        if(tecla==SDLK_ESCAPE) { tela_previa_wallpaper=0; preview_custom_path[0]='\0'; }
        else if((tecla==SDLK_LEFT || tecla==SDLK_RIGHT) && !preview_custom_path[0]) {
            ui_som_tocar(UI_SOM_TROCA);
            if(total_wallpapers) {
                preview_indice=(preview_indice+(tecla==SDLK_RIGHT?1:-1)+total_wallpapers)%total_wallpapers;
                if(preview_textura) SDL_DestroyTexture(preview_textura);
                preview_textura=IMG_LoadTexture(renderer,wallpapers[preview_indice]);
            }
        } else if(tecla_confirmar(tecla)) {
            if(preview_custom_path[0]) atualizar_wallpaper(preview_custom_path,1);
            else if(total_wallpapers) {
                atualizar_wallpaper(wallpapers[preview_indice],1);
                wallpaper_indice=preview_indice;
            }
            tela_previa_wallpaper=0; preview_custom_path[0]='\0';
        }
        return MENU_ACAO_NENHUMA;
    }
    if (tela_estilo_lateral) {
        if(tecla==SDLK_ESCAPE || tecla==SDLK_LEFT)
            tela_estilo_lateral=0;
        else if(tecla==SDLK_UP) {
            opcao_estilo_lateral=(opcao_estilo_lateral+4)%5;
            ui_som_tocar(UI_SOM_NAVEGACAO);
        } else if(tecla==SDLK_DOWN) {
            opcao_estilo_lateral=(opcao_estilo_lateral+1)%5;
            ui_som_tocar(UI_SOM_NAVEGACAO);
        } else if(tecla_confirmar(tecla) || tecla==SDLK_RIGHT) {
            if(opcao_estilo_lateral!=4) ui_som_tocar(UI_SOM_TROCA);
            switch(opcao_estilo_lateral) {
            case 0:
                if(total_pacotes_usuario>0) {
                    proximo_pacote_usuario();
                    carregar_icones_preset();
                } else ui_menu_aviso("Adicione um pacote completo em ui/styles/");
                break;
            case 1:
                sidebar_pilula=!sidebar_pilula;
                break;
            case 2:
                consoles_cabecalho_visivel=!consoles_cabecalho_visivel;
                if(!consoles_cabecalho_visivel && foco==FOCO_CABECALHO)
                    foco=FOCO_CARROSSEL;
                break;
            case 3:
                escanear_pacotes_usuario();
                carregar_icones_preset();
                ui_menu_aviso("Lista de estilos atualizada");
                break;
            case 4:
                tela_estilo_lateral=0;
                break;
            }
            salvar_config();
        }
        return MENU_ACAO_NENHUMA;
    }
    if (tela_aparencia) {
        if(tecla==SDLK_ESCAPE || tecla==SDLK_LEFT) tela_aparencia=0;
        else if(tecla==SDLK_UP) { opcao_aparencia=(opcao_aparencia+TOTAL_OPCOES_APARENCIA-1)%TOTAL_OPCOES_APARENCIA; ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if(tecla==SDLK_DOWN) { opcao_aparencia=(opcao_aparencia+1)%TOTAL_OPCOES_APARENCIA; ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if(tecla_confirmar(tecla) || tecla==SDLK_RIGHT) {
            if (opcao_aparencia != 7) ui_som_tocar(UI_SOM_TROCA);
            switch(opcao_aparencia) {
            case 0:
                contraste_automatico=!contraste_automatico;
                break;
            case 1:
                if(contraste_automatico) {
                    manual_preset=auto_preset;
                    contraste_automatico=0;
                } else manual_preset=(Preset)((manual_preset+1)%3);
                break;
            case 2:
                modo_texto=(ModoTexto)((modo_texto+1)%3);
                break;
            case 3:
                ui_game_view_set(ui_game_view_get()==GAME_VIEW_CARTRIDGE
                                  ? GAME_VIEW_CARD : GAME_VIEW_CARTRIDGE);
                break;
            case 4:
                listar_wallpapers();
                preview_indice=wallpaper_indice;
                preview_custom_path[0]='\0';
                if(preview_textura) SDL_DestroyTexture(preview_textura);
                preview_textura=total_wallpapers
                    ? IMG_LoadTexture(renderer,wallpapers[preview_indice]) : NULL;
                if(total_wallpapers) tela_previa_wallpaper=1;
                else ui_menu_aviso("Nenhum wallpaper disponivel");
                break;
            case 5:
                atualizar_wallpaper("ui/wallpapers/bk-01.jpg",1);
                break;
            case 6:
                escanear_pacotes_usuario();
                tela_estilo_lateral=1;
                opcao_estilo_lateral=0;
                break;
            case 7:
                tela_aparencia=0;
                break;
            }
            carregar_icones_preset();
            carregar_setas_preset();
            salvar_config();
        }
        return MENU_ACAO_NENHUMA;
    }
    if (tela_configuracoes) {
        if(tecla==SDLK_LEFT) { tela_configuracoes=0; return MENU_ACAO_NENHUMA; }
        if(tecla==SDLK_UP) { opcao_configuracoes=(opcao_configuracoes+total_opcoes_config-1)%total_opcoes_config; ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if(tecla==SDLK_DOWN) { opcao_configuracoes=(opcao_configuracoes+1)%total_opcoes_config; ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if(tecla==SDLK_ESCAPE) tela_configuracoes=0;
        else if(tecla_confirmar(tecla)) {
            switch(opcao_configuracoes) {
            case 0: mostrar_todos_consoles(); break;
            case 1: tela_configuracoes=0; break;
            }
        }
        return MENU_ACAO_NENHUMA;
    }

    /* E abre o editor somente no modo digital, nunca nos cartuchos. */
    if (tecla==SDLK_e && foco==FOCO_CARROSSEL &&
        ui_game_view_get()==GAME_VIEW_CARD && carousel_total_visiveis()>0) {
        if (ui_cards_iniciar_edicao(obter_caminho_jogo_atual()))
            ui_som_tocar(UI_SOM_TROCA);
        return MENU_ACAO_NENHUMA;
    }

    /* Acesso rapido ao menu lateral mesmo quando esta recolhido.
       PageUp/PageDown movem o foco pela lista sem abrir a pilula.
       Segurar PageUp ou PageDown + seta esquerda entra na lateral
       sem mexer no console/cartucho selecionado. */
    if (tecla == SDLK_PAGEUP || tecla == SDLK_PAGEDOWN) {
        sidebar_visivel=1;
        int passo = tecla == SDLK_PAGEUP ? -1 : 1;
        if (foco != FOCO_SIDEBAR) {
            foco = FOCO_SIDEBAR;
        } else {
            item_lateral = (item_lateral + passo + TOTAL_ACOES_LATERAIS)
                         % TOTAL_ACOES_LATERAIS;
        }
        ui_som_tocar(UI_SOM_NAVEGACAO);
        return MENU_ACAO_NENHUMA;
    }
    if (tecla == SDLK_LEFT) {
        const Uint8 *teclas = SDL_GetKeyboardState(NULL);
        if (teclas && (teclas[SDL_SCANCODE_PAGEUP] ||
                       teclas[SDL_SCANCODE_PAGEDOWN])) {
            sidebar_visivel=1;
            foco = FOCO_SIDEBAR;
            return MENU_ACAO_NENHUMA;
        }
    }

    if (tecla == SDLK_TAB) {
        /* Tab: ocultar/mostrar; ao restaurar, a lateral recebe foco. */
        sidebar_visivel = !sidebar_visivel;
        foco = sidebar_visivel ? FOCO_SIDEBAR : FOCO_CARROSSEL;
        ui_som_tocar(UI_SOM_NAVEGACAO);
        return MENU_ACAO_NENHUMA;
    }

    if (foco == FOCO_SIDEBAR) {
        if (tecla == SDLK_UP) {
            item_lateral = (item_lateral + TOTAL_ACOES_LATERAIS - 1) % TOTAL_ACOES_LATERAIS;
            ui_som_tocar(UI_SOM_NAVEGACAO);
        } else if (tecla == SDLK_DOWN) {
            item_lateral = (item_lateral + 1) % TOTAL_ACOES_LATERAIS;
            ui_som_tocar(UI_SOM_NAVEGACAO);
        } else if (tecla == SDLK_RIGHT || tecla == SDLK_ESCAPE) {
            foco = FOCO_CARROSSEL;
        } else if (tecla == SDLK_LEFT) {
            foco = FOCO_CARROSSEL;
        } else if (tecla_confirmar(tecla)) {
            switch (item_lateral) {
            case LATERAL_SAIR:
                return MENU_ACAO_SAIR;
            case LATERAL_CONFIGURACOES:
                tela_configuracoes = 1;
                opcao_configuracoes = 0;
                break;
            case LATERAL_CUSTOMIZACAO:
                tela_aparencia=1; opcao_aparencia=0;
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
        if (tecla == SDLK_LEFT) { trocar_console(-1); ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if (tecla == SDLK_RIGHT) { trocar_console(+1); ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if (tecla == SDLK_DOWN || tecla_confirmar(tecla)) foco = FOCO_CARROSSEL;
        else if (tecla == SDLK_h) {
            if (modificadores & KMOD_SHIFT) mostrar_todos_consoles();
            else if (total_visiveis > 0) ocultar_console_atual();
        }
        break;

    case FOCO_CARROSSEL:
        if (tecla == SDLK_LEFT) { mover_carousel_esquerda(); if(carousel_total_visiveis()>1) ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if (tecla == SDLK_RIGHT) { mover_carousel_direita(); if(carousel_total_visiveis()>1) ui_som_tocar(UI_SOM_NAVEGACAO); }
        else if (tecla == SDLK_UP) foco = FOCO_CABECALHO;
        else if (tecla == SDLK_DOWN) foco = FOCO_CARROSSEL;
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

/* Clique em qualquer cartucho lateral move o mesmo indice do titulo. */
MenuAcao ui_menu_clique(int mx,int my) {
    int vw=0,vh=0;
    obter_tamanho_ui(&vw,&vh);
    if(vw<=0 || vh<=0) return MENU_ACAO_NENHUMA;
    if(ui_cards_editando() || tela_previa_wallpaper) return MENU_ACAO_NENHUMA;
    if(tela_aparencia || tela_configuracoes || tela_estilo_lateral) {
        int sub=tela_estilo_lateral;
        int custom=tela_aparencia && !sub;
        int n=sub?5:(custom?TOTAL_OPCOES_APARENCIA:2);
        int pw=sub?440:(custom?360:340);
        if(pw>vw-42) pw=vw-42;
        int ph=sub?334:(custom?ALTURA_MODAL_APARENCIA:178);
        if(ph>vh-46) ph=vh-46;
        int x=(vw-pw)/2;
        int y=(vh-ph)/2;
        int passo=(custom||sub)?50:45;
        if(mx>=x+11 && mx<x+pw-11 && my>=y+59 && my<y+59+n*passo) {
            int item=(my-y-59)/passo;
            if(sub) opcao_estilo_lateral=item;
            else if(custom) opcao_aparencia=item;
            else opcao_configuracoes=item;
            return ui_menu_tecla(SDLK_RETURN,0);
        }
        return MENU_ACAO_NENHUMA;
    }
    SDL_Rect painel_sidebar; int lista_y=0, h_item_sidebar=0;
    obter_geometria_sidebar(vh,&painel_sidebar,&lista_y,&h_item_sidebar,NULL);
    if(sidebar_visivel && mx>=painel_sidebar.x && mx<painel_sidebar.x+painel_sidebar.w &&
       my>=painel_sidebar.y && my<painel_sidebar.y+painel_sidebar.h) {
        for(int i=0;i<TOTAL_ACOES_LATERAIS;++i) {
            int cy=lista_y+i*h_item_sidebar+h_item_sidebar/2;
            int half=h_item_sidebar/2;
            if(my>=cy-half && my<cy+half) {
                if (item_lateral != i || foco != FOCO_SIDEBAR) ui_som_tocar(UI_SOM_NAVEGACAO);
                item_lateral=i;
                foco=FOCO_SIDEBAR;
                return ui_menu_tecla(SDLK_RETURN,0);
            }
        }
        foco=FOCO_SIDEBAR;
        return MENU_ACAO_NENHUMA;
    }
    SDL_Rect faixa={SIDEBAR_RECOLHIDA,ALTURA_CABECALHO,vw-SIDEBAR_RECOLHIDA,
        vh-ALTURA_CABECALHO-ALTURA_TITULO-AREA_ANIMACAO_FUTURA};
    if(mx<faixa.x || mx>faixa.x+faixa.w || my<faixa.y || my>faixa.y+faixa.h+ALTURA_TITULO)
        return MENU_ACAO_NENHUMA;
    if(!carousel_total_visiveis()) return MENU_ACAO_NENHUMA;
    int cx=faixa.x+faixa.w/2;
    int cy=faixa.y+(int)(faixa.h*0.47f);
    int cart=(int)(faixa.h*0.62f);
    if(cart>(int)(faixa.w*0.30f)) cart=(int)(faixa.w*0.30f);
    if(cart>340) cart=340;
    if(cart<64) cart=64;
    int zona=cart*46/100;
    foco=FOCO_CARROSSEL;
    if(mx<cx-zona) { mover_carousel_esquerda(); if(carousel_total_visiveis()>1) ui_som_tocar(UI_SOM_NAVEGACAO); }
    else if(mx>cx+zona) { mover_carousel_direita(); if(carousel_total_visiveis()>1) ui_som_tocar(UI_SOM_NAVEGACAO); }
    else if(my>=cy-cart/2 && my<cy+cart/2) return MENU_ACAO_INICIAR_JOGO;
    return MENU_ACAO_NENHUMA;
}
void ui_menu_rolagem(int direcao) {
    if(ui_cards_editando() || tela_previa_wallpaper ||
       tela_aparencia || tela_configuracoes || tela_estilo_lateral ||
       !direcao) return;
    foco=FOCO_CARROSSEL;
    if(direcao>0) mover_carousel_esquerda();
    else mover_carousel_direita();
    if(carousel_total_visiveis()>1) ui_som_tocar(UI_SOM_NAVEGACAO);
}


/* Arrastar uma imagem para a janela abre a previa, sem modificar preferencias ate confirmar. */
void ui_menu_wallpaper_solto(const char *caminho) {
    if(!caminho || !*caminho || !extensao_imagem(caminho)) {
        ui_menu_aviso("Solte um arquivo PNG ou JPG"); return;
    }
    SDL_Texture *tex=IMG_LoadTexture(renderer,caminho);
    if(!tex) {ui_menu_aviso("Imagem invalida");return;}
    if(preview_textura) SDL_DestroyTexture(preview_textura);
    preview_textura=tex;
    snprintf(preview_custom_path,sizeof(preview_custom_path),"%s",caminho);
    tela_previa_wallpaper=1;
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

/* Bordas suaves reais: cobertura subpixel da curva numa textura RGBA.
   Cache de geometrias evita reconstruir os paineis a cada frame. */
#define LIMITE_CACHE_ARREDONDADO 96
typedef struct {
    int w, h, raio, borda;
    SDL_Color cor;
    SDL_Texture *tex;
} EntradaArredondado;
static EntradaArredondado cache_arredondados[LIMITE_CACHE_ARREDONDADO];
static int total_cache_arredondado=0;
static int proximo_cache_arredondado=0;

static float limite01(float n) {
    return n<0.0f?0.0f:(n>1.0f?1.0f:n);
}
static float cobertura_arredondada(float px,float py,int largura,int altura,int raio) {
    float mx=largura*0.5f, my=altura*0.5f;
    float dx=fabsf(px-mx)-(mx-raio);
    float dy=fabsf(py-my)-(my-raio);
    float dx_externo=dx>0.0f?dx:0.0f;
    float dy_externo=dy>0.0f?dy:0.0f;
    float externo=sqrtf(dx_externo*dx_externo+dy_externo*dy_externo);
    float interno=fminf(fmaxf(dx,dy),0.0f);
    float distancia=externo+interno-raio;
    return limite01(0.5f-distancia);
}
static void limpar_cache_arredondado(void) {
    for(int i=0;i<total_cache_arredondado;++i) {
        if(cache_arredondados[i].tex) SDL_DestroyTexture(cache_arredondados[i].tex);
        cache_arredondados[i].tex=NULL;
    }
    total_cache_arredondado=proximo_cache_arredondado=0;
}
static SDL_Texture *textura_arredondada(int w,int h,int raio,SDL_Color cor,int borda) {
    for(int i=0;i<total_cache_arredondado;++i) {
        EntradaArredondado *e=&cache_arredondados[i];
        if(e->w==w && e->h==h && e->raio==raio && e->borda==borda &&
           memcmp(&e->cor,&cor,sizeof(cor))==0) return e->tex;
    }
    SDL_Surface *superficie=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
    if(!superficie) return NULL;
    if(SDL_LockSurface(superficie)!=0) {SDL_FreeSurface(superficie);return NULL;}
    int interno_w=w-2*borda, interno_h=h-2*borda;
    int interno_raio=raio-borda;
    if(interno_raio<0) interno_raio=0;
    for(int y=0;y<h;++y) {
        Uint32 *linha=(Uint32*)((Uint8*)superficie->pixels+y*superficie->pitch);
        for(int x=0;x<w;++x) {
            float px=x+0.5f, py=y+0.5f;
            float cobertura=cobertura_arredondada(px,py,w,h,raio);
            if(borda>0 && interno_w>0 && interno_h>0) {
                float interior=cobertura_arredondada(px-borda,py-borda,interno_w,interno_h,interno_raio);
                cobertura=limite01(cobertura-interior);
            }
            Uint8 alpha=(Uint8)(cor.a*cobertura+0.5f);
            linha[x]=SDL_MapRGBA(superficie->format,cor.r,cor.g,cor.b,alpha);
        }
    }
    SDL_UnlockSurface(superficie);
    SDL_Texture *tex=SDL_CreateTextureFromSurface(renderer,superficie);
    SDL_FreeSurface(superficie);
    if(!tex) return NULL;
    SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
    int indice;
    if(total_cache_arredondado<LIMITE_CACHE_ARREDONDADO) indice=total_cache_arredondado++;
    else {
        indice=proximo_cache_arredondado++%LIMITE_CACHE_ARREDONDADO;
        SDL_DestroyTexture(cache_arredondados[indice].tex);
    }
    cache_arredondados[indice]=(EntradaArredondado){w,h,raio,borda,cor,tex};
    return tex;
}
static void arredondado(SDL_Rect a,int raio,SDL_Color cor,int borda) {
    if(a.w<=0 || a.h<=0) return;
    if(raio>a.w/2) raio=a.w/2;
    if(raio>a.h/2) raio=a.h/2;
    if(raio<0) raio=0;
    if(borda<0) borda=0;
    SDL_Texture *tex=textura_arredondada(a.w,a.h,raio,cor,borda);
    if(tex) SDL_RenderCopy(renderer,tex,NULL,&a);
}
static void desenhar_capsula(SDL_Rect r,SDL_Color fundo,SDL_Color borda) {
    arredondado(r,r.h/2,fundo,0);
    arredondado(r,r.h/2,borda,1);
}
/* Hora em texto e icone original da bateria, sem capsula ou relogio
   decorativo. Posicionamento calculado a partir da largura real dos textos. */
static void desenhar_relogio(int vw) {
    time_t agora=time(NULL);
    struct tm *tm=localtime(&agora);
    if(!tm) return;
    char horario[32];
    strftime(horario,sizeof(horario),"%H:%M",tm);
    int pct=-1;
#ifdef _WIN32
    SYSTEM_POWER_STATUS energia;
    if(GetSystemPowerStatus(&energia) && energia.BatteryLifePercent!=255)
        pct=(int)energia.BatteryLifePercent;
#endif
    char porcentagem[16]="";
    if(pct>=0) snprintf(porcentagem,sizeof(porcentagem),"%d%%",pct);
    const int margem_direita=28, espacamento=18, largura_bateria=28;
    const int largura_hora=ui_texto_largura(horario,1);
    const int largura_percentual=pct>=0?ui_texto_largura(porcentagem,1):0;
    const int largura_total=largura_hora+(pct>=0?espacamento+largura_bateria+8+largura_percentual:0);
    const int inicio_x=vw-margem_direita-largura_total;
    const int texto_y=19;
    const int altura_texto=ui_texto_altura(1);
    SDL_Rect faixa={inicio_x-5,texto_y-3,largura_total+10,altura_texto+8};
    SDL_Color sombra, tinta=cor_legenda_local(faixa,0,&sombra);
    ui_texto_desenhar(horario,inicio_x,texto_y,1,tinta,sombra);
    if(pct<0) return;

    const int bateria_x=inicio_x+largura_hora+espacamento;
    const int bateria_y=texto_y+(altura_texto-13)/2;
    SDL_Rect corpo={bateria_x,bateria_y,25,13};
    /* Mantem a bateria anterior: corpo com cantos arredondados,
       terminal lateral e preenchimento colorido. */
    arredondado(corpo,3,(SDL_Color){tinta.r,tinta.g,tinta.b,230},1);
    preencher((SDL_Rect){corpo.x+26,corpo.y+4,2,5},
             (SDL_Color){tinta.r,tinta.g,tinta.b,215});
    int capacidade=pct;
    if(capacidade<0) capacidade=0;
    if(capacidade>100) capacidade=100;
    SDL_Color carga=capacidade<20 ? (SDL_Color){244,108,101,245}
                                 : (SDL_Color){110,210,160,240};
    SDL_Rect nivel={corpo.x+2,corpo.y+2,21*capacidade/100,9};
    if(nivel.w>0) arredondado(nivel,2,carga,0);
    ui_texto_desenhar(porcentagem,bateria_x+largura_bateria+8,texto_y,1,
                     tinta,sombra);
}

/* Mantido por compatibilidade com o frontend; o rodape visual foi removido. */
void ui_menu_controle_conectado(int sim) { (void)sim; }

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
        SDL_RenderCopy(renderer, wallpaper, NULL, &(SDL_Rect){0,0,vw,vh});
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

    SDL_RenderCopy(renderer, wallpaper, &origem, &(SDL_Rect){0,0,vw,vh});
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

    if (total_visiveis == 0) {
        ui_texto_desenhar_centro("Nenhum console ativo", x_area + vw / 2, 30, 2,
                                 cor_texto_ui(), cor_sombra_ui());
        ui_texto_desenhar_centro("Confira a pasta cores/", x_area + vw / 2, 52, 2,
                                 cor_texto_suave_ui(), cor_sombra_ui());
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

        int icone_w = selecionado ? ICONE_MAX_W : ICONE_MAX_W - 16;
        int icone_h = selecionado ? ICONE_MAX_H : ICONE_MAX_H - 10;
        SDL_Rect caixa_icone = { slot.x + (slot.w - icone_w) / 2,
                                 slot.y + (slot.h - icone_h) / 2,
                                 icone_w, icone_h };

        if (item->icone) {
            desenhar_encaixado(item->icone, item->icone_w, item->icone_h,
                               caixa_icone, selecionado ? 255 : 175);
        } else {
            /* Sem icone: mostra o identificador do console. */
            char sigla[TAM_ID];
            snprintf(sigla, sizeof(sigla), "%s", item->cfg->id);
            for (char *c = sigla; *c; ++c)
                if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
            ui_texto_desenhar_centro(sigla, slot.x + slot.w / 2,
                                     slot.y + (slot.h - ui_texto_altura(2)) / 2, 2,
                                     cor_texto_ui(), cor_sombra_ui());
        }

        /* Nome curto (id em maiusculas) embaixo de cada icone; o selecionado ganha destaque. */
        char rotulo[TAM_ID];
        snprintf(rotulo, sizeof(rotulo), "%s", item->cfg->id);
        for (char *c = rotulo; *c; ++c)
            if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
        int rotulo_y = y0 + SLOT_H + 4;
        int rotulo_largura = ui_texto_largura(rotulo, 2);
        SDL_Rect faixa_rotulo = {slot.x + (slot.w - rotulo_largura) / 2,
                                 rotulo_y, rotulo_largura, ui_texto_altura(2)};
        SDL_Color sombra_rotulo;
        SDL_Color tinta_rotulo = cor_legenda_local(faixa_rotulo, !selecionado,
                                                  &sombra_rotulo);
        ui_texto_desenhar_centro(rotulo, slot.x + slot.w / 2, rotulo_y, 2,
                                 tinta_rotulo, sombra_rotulo);
    }
}

static void desenhar_linha_titulo(int x_area, int vw, int y) {

    if (carousel_total_visiveis() == 0) return;

    char bruto[256], nome[256];
    carousel_nome_jogo_atual(bruto, sizeof(bruto));

    int max_texto = vw - 2 * (SETA_TAM + 36);
    ui_texto_ajustar(bruto, max_texto, 2, nome, sizeof(nome));
    int titulo_largura = ui_texto_largura(nome, 2);
    SDL_Rect faixa_titulo = { x_area + (vw - titulo_largura) / 2, y - 15,
                              titulo_largura, ui_texto_altura(2) };
    SDL_Color sombra_titulo;
    SDL_Color tinta_titulo = cor_legenda_local(faixa_titulo, 0, &sombra_titulo);
    ui_texto_desenhar_centro(nome, x_area + vw / 2, y - 15, 2,
                             tinta_titulo, sombra_titulo);

    if (carousel_total_visiveis() > 1) {
        int meio = ui_texto_largura(nome, 2) / 2;
        Uint8 alfa = foco == FOCO_CARROSSEL ? 255 : 130;
        SDL_Rect esq = { x_area + vw / 2 - meio - 16 - SETA_TAM, y - 10, SETA_TAM, SETA_TAM };
        SDL_Rect dir = { x_area + vw / 2 + meio + 16, y - 10, SETA_TAM, SETA_TAM };

        desenhar_encaixado(seta_esq, 512, 512, esq, alfa);
        desenhar_encaixado(seta_dir, 512, 512, dir, alfa);
    }
}

/* Fallback vetorial: os PNGs ui/styles/icons podem nao estar versionados.
   O desenho usa SDL2 e respeita os presets de cor sem dependencia externa. */
static void icone_linha(int x1,int y1,int x2,int y2) {
    SDL_RenderDrawLine(renderer,x1,y1,x2,y2);
}
static void desenhar_icone_fallback(int indice,SDL_Rect caixa,SDL_Color cor) {
    int x=caixa.x,y=caixa.y,w=caixa.w,h=caixa.h;
    int l=x+3,r=x+w-4,t=y+3,b=y+h-4,cx=x+w/2,cy=y+h/2;
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,cor.r,cor.g,cor.b,255);
    switch(indice) {
    case LATERAL_CONQUISTAS: {
        SDL_Rect ta={x+7,y+4,w-14,h/2}; SDL_RenderDrawRect(renderer,&ta);
        icone_linha(l,t+3,x+7,t+3); icone_linha(r,t+3,x+w-7,t+3);
        icone_linha(cx,cy,cx,b-3); icone_linha(cx-6,b-2,cx+6,b-2);
        break;
    }
    case LATERAL_CONFIGURACOES: {
        SDL_Rect ex={x+5,y+5,w-10,h-10};
        SDL_Rect in={x+10,y+10,w-20,h-20};
        SDL_RenderDrawRect(renderer,&ex); SDL_RenderDrawRect(renderer,&in);
        icone_linha(cx,t,cx,y+5); icone_linha(cx,b-2,cx,b);
        icone_linha(l,cy,x+5,cy); icone_linha(r-2,cy,r,cy);
        break;
    }
    case LATERAL_FILTROS:
        icone_linha(l,t,r,t); icone_linha(l,t,cx-3,cy+2);
        icone_linha(r,t,cx+3,cy+2); icone_linha(cx-3,cy+2,cx-3,b);
        icone_linha(cx+3,cy+2,cx+3,b); icone_linha(cx-3,b,cx+3,b);
        break;
    case LATERAL_FAVORITOS:
        icone_linha(cx,b,l,cy-2); icone_linha(cx,b,r,cy-2);
        icone_linha(l,cy-2,l+2,t+4); icone_linha(l+2,t+4,cx,t+7);
        icone_linha(cx,t+7,r-2,t+4); icone_linha(r-2,t+4,r,cy-2);
        break;
    case LATERAL_CUSTOMIZACAO:
        icone_linha(l,b,r-3,t+2); icone_linha(l+2,b,cx-5,b);
        icone_linha(r-3,t+2,r,t+7); icone_linha(r,t+7,r-5,t+10);
        icone_linha(cx-7,cy+4,cx-3,cy+8);
        break;
    case LATERAL_SAIR: {
        SDL_Rect porta={x+9,y+5,w-14,h-10}; SDL_RenderDrawRect(renderer,&porta);
        icone_linha(l,cy,cx+3,cy); icone_linha(cx,cy-5,cx+5,cy);
        icone_linha(cx+5,cy,cx,cy+5);
        break;
    }
    }
}

/* Contorno por linhas e arcos: nao depende da mascara RGBA da textura
   arredondada, que em alguns backends cobria a legenda selecionada. */
/* Contorno anti-aliased de dois pixels logicos, sem preenchimento.
   Usa cobertura subpixel da textura ja existente (apenas a borda).
   Os preenchimentos escuros antigos foram removidos dos itens e modais. */
/* Distancia com sinal ate a borda de um retangulo arredondado (negativa
   dentro). Mesmo calculo de cobertura_arredondada, mas em float. */
static float distancia_retangulo_arredondado(float px,float py,float w,float h,
                                             float raio) {
    float mx=w*0.5f,my=h*0.5f;
    float dx=fabsf(px-mx)-(mx-raio);
    float dy=fabsf(py-my)-(my-raio);
    float fx=dx>0.0f?dx:0.0f,fy=dy>0.0f?dy:0.0f;
    return sqrtf(fx*fx+fy*fy)+fminf(fmaxf(dx,dy),0.0f)-raio;
}

/* Contorno anti-aliased, SEM preenchimento: cada pixel recebe alfa pela
   cobertura do anel (borda externa menos borda interna), entao nao ha
   falhas nos cantos nem trechos "tracejados". Desenha somente pontos da
   propria borda; o miolo nunca e tocado. Espessura em pixels logicos. */
#define ESPESSURA_SIDEBAR 1.8f  /* ~traco dos icones: ~30px/512 * 29px */
static void contorno_selecao_esp(SDL_Rect r,int raio,SDL_Color cor,float esp) {
    if(r.w<=2 || r.h<=2) return;
    if(raio>r.w/2) raio=r.w/2;
    if(raio>r.h/2) raio=r.h/2;
    if(raio<2) raio=2;

    if(esp<1.0f) esp=1.0f;
    float raio_f=(float)raio;
    float raio_in=raio_f-esp;
    if(raio_in<0.0f) raio_in=0.0f;
    float w_in=(float)r.w-2.0f*esp,h_in=(float)r.h-2.0f*esp;

    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    for(int y=0;y<r.h;++y) {
        for(int x=0;x<r.w;++x) {
            float px=x+0.5f,py=y+0.5f;
            float ext=limite01(0.5f-distancia_retangulo_arredondado(
                                         px,py,(float)r.w,(float)r.h,raio_f));
            if(ext<=0.0f) continue;
            float inte=0.0f;
            if(w_in>0.0f && h_in>0.0f)
                inte=limite01(0.5f-distancia_retangulo_arredondado(
                                       px-esp,py-esp,w_in,h_in,raio_in));
            float cobertura=limite01(ext-inte);
            Uint8 a=(Uint8)(cor.a*cobertura+0.5f);
            if(a==0) continue;
            SDL_SetRenderDrawColor(renderer,cor.r,cor.g,cor.b,a);
            SDL_RenderDrawPoint(renderer,r.x+x,r.y+y);
        }
    }
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
}

/* Demais usos (modais/listas) mantem o traco fino de antes. */
static void contorno_selecao(SDL_Rect r,int raio,SDL_Color cor) {
    contorno_selecao_esp(r,raio,cor,1.5f);
}

static void desenhar_sidebar(int vh) {
    const Preset preset=preset_ativo();
    SDL_Rect painel;
    int lista_y=0,h_item=0;
    obter_geometria_sidebar(vh,&painel,&lista_y,&h_item,NULL);
    /* Vidro opcional: os icones podem flutuar diretamente no wallpaper. */
    SDL_Color vidro = preset==PRESET_LIGHT
        ? (SDL_Color){25,37,73,142}
        : (SDL_Color){221,238,251,148};
    SDL_Color borda = preset==PRESET_LIGHT
        ? (SDL_Color){218,231,255,100}
        : (SDL_Color){255,255,255,153};
    if(sidebar_pilula) {
        arredondado((SDL_Rect){painel.x+2,painel.y+3,painel.w,painel.h},
                    painel.w/2,(SDL_Color){12,23,45,22},0);
        desenhar_capsula(painel,vidro,borda);
    }
    for(int i=0;i<TOTAL_ACOES_LATERAIS;++i) {
        int cy=lista_y+i*h_item+h_item/2;
        SDL_Rect item={painel.x+8,cy-21,painel.w-16,42};
        SDL_Color cor=icone_cor(i);
        if(i==LATERAL_SAIR && sidebar_pilula)
            preencher((SDL_Rect){painel.x+15,cy-h_item/2-4,
                                  painel.w-30,1},
                     preset==PRESET_LIGHT ? (SDL_Color){228,235,255,82}
                                         : (SDL_Color){43,65,92,76});
        if(item_lateral==i) {
            SDL_Color destaque = preset==PRESET_COLORFUL
                ? (SDL_Color){0xC8,0x46,0x82,240}
                : (SDL_Color){cor.r,cor.g,cor.b,235};
            /* Moldura sem preenchimento para nao escurecer os icones. */
            contorno_selecao_esp(item,16,destaque,ESPESSURA_SIDEBAR);
        }
        /* Compensa levemente as margens transparentes dos PNGs originais:
           trofeu e coracao tem silhouette menor que filtro/configuracoes. */
        int lado=SIDEBAR_ICON;
        if(i==LATERAL_FAVORITOS) lado+=1;
        SDL_Rect icone={painel.x+(painel.w-lado)/2,
                        cy-lado/2,lado,lado};
        if(icones_laterais[i]) {
            int tw=0,th=0;
            SDL_QueryTexture(icones_laterais[i],NULL,NULL,&tw,&th);
            desenhar_encaixado(icones_laterais[i],tw,th,icone,255);
        } else {
            desenhar_icone_fallback(i,icone,cor);
        }
    }
}

void ui_menu_desenhar(void) {
    const UiTema *tema = ui_tema_atual();
    int vw = 800, vh = 600;

    obter_tamanho_ui(&vw, &vh);
    if (vw <= 0) vw = 800;
    if (vh <= 0) vh = 600;

    desenhar_fundo(vw, vh);
    if(sidebar_visivel) desenhar_sidebar(vh);
    desenhar_relogio(vw);

    /* Nunca recalcular as posicoes do carrossel ao expandir a lateral. */
    int x_area = SIDEBAR_RECOLHIDA;
    int largura_area = vw - SIDEBAR_RECOLHIDA;
    if (largura_area < 1) largura_area = 1;

    if(consoles_cabecalho_visivel)
        desenhar_cabecalho(x_area, largura_area);

    SDL_Rect area = { x_area, ALTURA_CABECALHO, largura_area,
                      vh - ALTURA_CABECALHO - ALTURA_TITULO - AREA_ANIMACAO_FUTURA };
    if (area.h < 0) area.h = 0;

    if (total_visiveis > 0) {
        if (carousel_total_visiveis() > 0) {
            desenhar_carousel(area);
        } else {
            char pasta[160];
            snprintf(pasta, sizeof(pasta), "Coloque as ROMs em %s/", console_selecionado()->pasta_roms);
            ui_texto_desenhar_centro("Nenhuma ROM encontrada", x_area + largura_area / 2,
                                     area.y + area.h / 2 - 20, 2,
                                     cor_texto_ui(), cor_sombra_ui());
            ui_texto_desenhar_centro(pasta, x_area + largura_area / 2,
                                     area.y + area.h / 2 + 4, 2,
                                     cor_texto_suave_ui(), cor_sombra_ui());
        }
    }

    /* O titulo acompanha a geometria do cartucho ativo, nao uma zona independente. */
    int cart_box=(int)(area.h*0.62f);
    int limite=(int)(area.w*0.30f);
    if(cart_box>limite) cart_box=limite;
    if(cart_box>340) cart_box=340;
    if(cart_box<64) cart_box=64;
    int titulo_y=area.y+(int)(area.h*0.47f)+cart_box/2+29;
    desenhar_linha_titulo(x_area, largura_area, titulo_y);

    if (aviso[0] && SDL_GetTicks() < aviso_ate) {
        int larg = ui_texto_largura(aviso, 2) + 24;
        SDL_Rect faixa = { x_area + (largura_area - larg) / 2, area.y + 4,
                           larg, ui_texto_altura(2) + 12 };
        preencher(faixa, tema->painel);
        contorno(faixa, 1, tema->aviso);
        ui_texto_desenhar_centro(aviso, x_area + largura_area / 2, faixa.y + 6, 2,
                                 tema->aviso, (SDL_Color){ 0, 0, 0, 0 });
    }

    if (ui_cards_editando()) {
        UiCardAjuste ajuste=ui_cards_edicao_atual();
        SDL_Rect painel={x_area+65,vh-90,largura_area-130,79};
        arredondado(painel,16,(SDL_Color){15,22,41,220},0);
        arredondado(painel,16,(SDL_Color){229,240,255,130},1);
        const SDL_Color branco={244,248,255,255};
        const SDL_Color sem_sombra={0,0,0,0};
        char descricao[180];
        snprintf(descricao,sizeof(descricao),
                 "MODO: %s    ZOOM: %d%%    X: %d    Y: %d",
                 ajuste.encaixar?"ENCAIXAR":"PREENCHER",
                 ajuste.zoom_pct,ajuste.desloc_x,ajuste.desloc_y);
        ui_texto_desenhar_centro("AJUSTAR CAPA  |  SETAS: MOVER  +/-: ZOOM  F: MODO  R: RESET",
             x_area+largura_area/2,painel.y+9,1,branco,sem_sombra);
        ui_texto_desenhar_centro(descricao,
             x_area+largura_area/2,painel.y+30,1,branco,sem_sombra);
        ui_texto_desenhar_centro("ENTER: SALVAR     ESC: CANCELAR     (ZOOM EM PREENCHER)",
             x_area+largura_area/2,painel.y+51,1,branco,sem_sombra);
    }

    if(tela_previa_wallpaper) {
        SDL_Rect caixa={vw/2-230,vh/2-155,460,310};
        desenhar_capsula(caixa,(SDL_Color){22,27,43,240},(SDL_Color){255,255,255,120});
        if(preview_textura) {
            int w,h; SDL_QueryTexture(preview_textura,NULL,NULL,&w,&h);
            desenhar_encaixado(preview_textura,w,h,(SDL_Rect){caixa.x+20,caixa.y+25,420,225},255);
        }
        ui_texto_desenhar_centro("<  PREVIA DO WALLPAPER  >",vw/2,caixa.y+258,1,(SDL_Color){255,255,255,255},(SDL_Color){0,0,0,0});
        ui_texto_desenhar_centro("ENTER Aplicar   ESC Cancelar",vw/2,caixa.y+284,1,(SDL_Color){220,225,235,255},(SDL_Color){0,0,0,0});
    } else if(tela_configuracoes || tela_aparencia || tela_estilo_lateral) {
        const int sub=tela_estilo_lateral;
        const int custom=tela_aparencia && !sub;
        const int n=sub?5:(custom?TOTAL_OPCOES_APARENCIA:2);
        const int claro=preset_ativo()!=PRESET_LIGHT;
        SDL_Color tinta=claro?(SDL_Color){27,43,72,255}:(SDL_Color){245,247,255,255};
        SDL_Color fundo=claro?(SDL_Color){236,246,255,255}:(SDL_Color){22,35,72,255};
        SDL_Color borda=claro?(SDL_Color){255,255,255,240}:(SDL_Color){194,209,245,115};
        int pw=sub?440:(custom?360:340);
        if(pw>vw-42) pw=vw-42;
        int ph=sub?334:(custom?ALTURA_MODAL_APARENCIA:178);
        if(ph>vh-46) ph=vh-46;
        /* Modal centralizado com veu de fundo; nao depende da lateral. */
        SDL_Rect painel={(vw-pw)/2,(vh-ph)/2,pw,ph};
        preencher((SDL_Rect){0,0,vw,vh},(SDL_Color){8,12,24,claro?70:120});
        arredondado((SDL_Rect){painel.x+3,painel.y+5,painel.w,painel.h},25,
                    (SDL_Color){10,24,46,35},0);
        arredondado(painel,25,fundo,0);
        arredondado(painel,25,borda,1);
        ui_texto_desenhar(sub?"BARRA LATERAL":custom?"APARÊNCIA":"CONFIGURAÇÕES",painel.x+20,
                          painel.y+17,2,tinta,(SDL_Color){0,0,0,0});
        int altura_linha=(custom||sub)?50:45;
        for(int i=0;i<n;++i) {
            char nome[150];
            if(sub) {
                switch(i) {
                case 0:
                    if(pacote_usuario_selecionado[0])
                        snprintf(nome,sizeof(nome),"Pacote de icones: %s",
                                 pacote_usuario_selecionado);
                    else
                        snprintf(nome,sizeof(nome),"Pacote: Padrao (%s)",
                                 nomes_presets[preset_ativo()]);
                    break;
                case 1:
                    snprintf(nome,sizeof(nome),"Fundo lateral: %s",
                             sidebar_pilula?"Pilula":"Somente icones");
                    break;
                case 2:
                    snprintf(nome,sizeof(nome),"Consoles no topo: %s",
                             consoles_cabecalho_visivel?"Mostrar":"Ocultar");
                    break;
                case 3:
                    snprintf(nome,sizeof(nome),"Atualizar pacotes (%d)",
                             total_pacotes_usuario);
                    break;
                default: snprintf(nome,sizeof(nome),"Voltar");
                }
            } else if(custom) {
                switch(i) {
                case 0:
                    snprintf(nome,sizeof(nome),"Icones: %s (%s)",
                        contraste_automatico?"Automatico":"Manual",
                        nomes_presets[preset_ativo()]);
                    break;
                case 1:
                    if(contraste_automatico)
                        snprintf(nome,sizeof(nome),"Preset manual: Inativo");
                    else
                        snprintf(nome,sizeof(nome),"Preset manual: %s",
                                 nomes_presets[preset_ativo()]);
                    break;
                case 2:
                    snprintf(nome,sizeof(nome),"Cor dos titulos: %s",nomes_modos_texto[modo_texto]);
                    break;
                case 3:
                    snprintf(nome,sizeof(nome),"Visualização: %s",
                             ui_game_view_get()==GAME_VIEW_CARD?"Cards":"Cartuchos");
                    break;
                case 4: snprintf(nome,sizeof(nome),"Escolher wallpaper"); break;
                case 5: snprintf(nome,sizeof(nome),"Restaurar wallpaper padrão"); break;
                case 6: snprintf(nome,sizeof(nome),"Personalizar barra lateral"); break;
                default: snprintf(nome,sizeof(nome),"Voltar");
                }
            } else {
                snprintf(nome,sizeof(nome),"%s",i==0?"Restaurar consoles ocultos":"Voltar");
            }
            SDL_Rect linha={painel.x+11,painel.y+59+i*altura_linha,painel.w-22,altura_linha-6};
            if((sub?opcao_estilo_lateral:
                custom?opcao_aparencia:opcao_configuracoes)==i) {
                /* Sem camada escura atras da legenda selecionada. */
                SDL_Color borda_ativa=preset_ativo()==PRESET_COLORFUL
                    ? (SDL_Color){0xC8,0x46,0x82,235}
                    : claro ? (SDL_Color){38,68,116,225}
                            : (SDL_Color){207,220,255,230};
                contorno_selecao(linha,14,borda_ativa);
            }
            char nome_ajustado[150];
            ui_texto_ajustar(nome,linha.w-22,1,
                             nome_ajustado,sizeof(nome_ajustado));
            ui_texto_desenhar(nome_ajustado,linha.x+11,linha.y+10,1,
                              tinta,(SDL_Color){0,0,0,0});
        }

    }
}
