#include "ui_cards.h"
#include "ui_text.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CANTOS_PASSOS 7
#define VERTICES_MAX (1 + 4*(CANTOS_PASSOS+1))
#define INDICES_MAX (4*(CANTOS_PASSOS+1)*3)
#define AJUSTES_ARQUIVO "fofita_cards.cfg"
#define MAX_AJUSTES 4096

typedef struct {
    uint64_t chave;
    UiCardAjuste ajuste;
} RegistroCard;

static RegistroCard *registros = NULL;
static size_t total_registros = 0;
static int em_edicao = 0;
static uint64_t chave_edicao = 0;
static UiCardAjuste ajuste_edicao;

static int limitar(int valor,int minimo,int maximo) {
    return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}
static UiCardAjuste padrao(void) {
    UiCardAjuste a = {100,0,0,0};
    return a;
}
static void normalizar(UiCardAjuste *a) {
    a->zoom_pct=limitar(a->zoom_pct,100,250);
    a->desloc_x=limitar(a->desloc_x,-100,100);
    a->desloc_y=limitar(a->desloc_y,-100,100);
    a->encaixar=a->encaixar!=0;
}
/* FNV-1a de 64 bits do caminho completo relativo da ROM.
   Preserva ajustes mesmo quando o nome visivel e repetido. */
static uint64_t chave_rom(const char *rom) {
    uint64_t h=UINT64_C(14695981039346656037);
    for(const unsigned char *p=(const unsigned char *)rom; *p; ++p) {
        h^=*p;
        h*=UINT64_C(1099511628211);
    }
    return h;
}
static int localizar(uint64_t chave) {
    for(size_t i=0;i<total_registros;++i)
        if(registros[i].chave==chave) return (int)i;
    return -1;
}
static UiCardAjuste ajuste_de_rom(const char *rom) {
    if(!rom || !*rom) return padrao();
    uint64_t chave=chave_rom(rom);
    if(em_edicao && chave==chave_edicao) return ajuste_edicao;
    int i=localizar(chave);
    return i>=0 ? registros[i].ajuste : padrao();
}

void ui_cards_liberar_ajustes(void) {
    free(registros);
    registros=NULL;
    total_registros=0;
    em_edicao=0;
}
void ui_cards_carregar_ajustes(void) {
    ui_cards_liberar_ajustes();
    FILE *f=fopen(AJUSTES_ARQUIVO,"r");
    if(!f) return;
    char linha[128];
    while(fgets(linha,sizeof(linha),f) && total_registros<MAX_AJUSTES) {
        unsigned long long chave=0;
        UiCardAjuste a;
        if(sscanf(linha,"%llx %d %d %d %d",&chave,
                  &a.zoom_pct,&a.desloc_x,&a.desloc_y,&a.encaixar)!=5)
            continue;
        normalizar(&a);
        if(localizar((uint64_t)chave)>=0) continue;
        RegistroCard *novo=realloc(registros,(total_registros+1)*sizeof(*registros));
        if(!novo) break;
        registros=novo;
        registros[total_registros++]=(RegistroCard){(uint64_t)chave,a};
    }
    fclose(f);
}
int ui_cards_iniciar_edicao(const char *rom) {
    if(!rom || !*rom || em_edicao) return 0;
    chave_edicao=chave_rom(rom);
    ajuste_edicao=ajuste_de_rom(rom);
    em_edicao=1;
    return 1;
}
int ui_cards_editando(void) { return em_edicao; }
void ui_cards_edicao_mover(int x,int y) {
    if(!em_edicao) return;
    ajuste_edicao.desloc_x+=x;
    ajuste_edicao.desloc_y+=y;
    normalizar(&ajuste_edicao);
}
void ui_cards_edicao_zoom(int delta) {
    if(!em_edicao) return;
    ajuste_edicao.zoom_pct+=delta;
    normalizar(&ajuste_edicao);
}
void ui_cards_edicao_alternar_modo(void) {
    if(em_edicao) ajuste_edicao.encaixar=!ajuste_edicao.encaixar;
}
void ui_cards_edicao_restaurar(void) {
    if(em_edicao) ajuste_edicao=padrao();
}
UiCardAjuste ui_cards_edicao_atual(void) {
    return em_edicao ? ajuste_edicao : padrao();
}
void ui_cards_edicao_cancelar(void) {
    em_edicao=0;
}
int ui_cards_edicao_salvar(void) {
    if(!em_edicao) return 0;
    int i=localizar(chave_edicao);
    if(i<0) {
        if(total_registros>=MAX_AJUSTES) return 0;
        RegistroCard *novo=realloc(registros,(total_registros+1)*sizeof(*registros));
        if(!novo) return 0;
        registros=novo;
        i=(int)total_registros++;
        registros[i].chave=chave_edicao;
    }
    registros[i].ajuste=ajuste_edicao;
    FILE *f=fopen(AJUSTES_ARQUIVO,"w");
    if(!f) return 0;
    for(size_t j=0;j<total_registros;++j) {
        UiCardAjuste a=registros[j].ajuste;
        if(fprintf(f,"%016llx %d %d %d %d\n",
           (unsigned long long)registros[j].chave,
           a.zoom_pct,a.desloc_x,a.desloc_y,a.encaixar)<0) {
            fclose(f);
            return 0;
        }
    }
    if(fclose(f)!=0) return 0;
    em_edicao=0;
    return 1;
}

/* Poligono de cantos arredondados com UV, reaproveitado entre frames.
   O recorte nao modifica a textura original e nao cria copias. */
static void rounded(SDL_Renderer *r,SDL_Texture *imagem,SDL_Rect rect,
                    int raio,SDL_Color tinta,float u0,float v0,float u1,float v1) {
    if(rect.w<=0 || rect.h<=0) return;
    if(raio>rect.w/2) raio=rect.w/2;
    if(raio>rect.h/2) raio=rect.h/2;
    SDL_Vertex vertices[VERTICES_MAX];
    int indices[INDICES_MAX];
    float meio_x=rect.x+rect.w*0.5f;
    float meio_y=rect.y+rect.h*0.5f;
    vertices[0]=(SDL_Vertex){{meio_x,meio_y},tinta,
        {(u0+u1)*0.5f,(v0+v1)*0.5f}};
    int n=1;
    for(int c=0;c<4;++c) {
        float centro_x=(c==0||c==1)?rect.x+rect.w-raio:rect.x+raio;
        float centro_y=(c==0||c==3)?rect.y+raio:rect.y+rect.h-raio;
        for(int k=0;k<=CANTOS_PASSOS;++k) {
            float angulo=(-90.0f+c*90.0f+k*90.0f/CANTOS_PASSOS)*0.01745329252f;
            float px=centro_x+cosf(angulo)*raio;
            float py=centro_y+sinf(angulo)*raio;
            vertices[n++]=(SDL_Vertex){{px,py},tinta,
                {u0+(px-rect.x)/rect.w*(u1-u0),
                 v0+(py-rect.y)/rect.h*(v1-v0)}};
        }
    }
    int tri=0;
    for(int i=1;i<n;++i) {
        indices[tri++]=0;
        indices[tri++]=i;
        indices[tri++]=(i+1<n)?i+1:1;
    }
    SDL_RenderGeometry(r,imagem,vertices,n,indices,tri);
}
void ui_cards_desenhar(SDL_Renderer *r,SDL_Texture *capa,
                       const char *titulo,const char *rom,
                       SDL_Rect area,int intensidade,int selecionado) {
    if(!r || area.w<=0 || area.h<=0) return;
    int lado=area.w<area.h?area.w:area.h;
    SDL_Rect carta={area.x+(area.w-lado)/2,area.y+(area.h-lado)/2,lado,lado};
    int raio=lado/9;
    if(raio<6) raio=6;
    int borda=selecionado?4:2;
    if(selecionado) {
        SDL_Rect sombra={carta.x+4,carta.y+6,carta.w,carta.h};
        rounded(r,NULL,sombra,raio+1,(SDL_Color){8,14,25,130},0,0,1,1);
    }
    SDL_Color moldura=selecionado
        ? (SDL_Color){255,255,255,255}
        : (SDL_Color){223,231,247,115};
    rounded(r,NULL,carta,raio,moldura,0,0,1,1);
    SDL_Rect arte={carta.x+borda,carta.y+borda,
                   carta.w-2*borda,carta.h-2*borda};
    if(capa) {
        int tw=0,th=0;
        SDL_QueryTexture(capa,NULL,NULL,&tw,&th);
        if(tw>0 && th>0 && arte.w>0 && arte.h>0) {
            UiCardAjuste a=ajuste_de_rom(rom);
            Uint8 luz=(Uint8)limitar(intensidade,0,255);
            if(a.encaixar) {
                /* Modo Encaixar: a arte fica inteira, sem distorcao. */
                rounded(r,NULL,arte,raio-borda,(SDL_Color){27,41,64,255},0,0,1,1);
                float escala=fminf((float)arte.w/tw,(float)arte.h/th);
                int dw=(int)(tw*escala+0.5f);
                int dh=(int)(th*escala+0.5f);
                SDL_Rect destino={arte.x+(arte.w-dw)/2,
                                  arte.y+(arte.h-dh)/2,dw,dh};
                SDL_SetTextureColorMod(capa,luz,luz,luz);
                SDL_RenderCopy(r,capa,NULL,&destino);
                SDL_SetTextureColorMod(capa,255,255,255);
            } else {
                /* Preencher: recorte quadrado automatico + pan/zoom manual.
                   Pan positivo move a ARTE na direcao da seta. */
                float bw=tw>th?(float)th/tw:1.0f;
                float bh=th>tw?(float)tw/th:1.0f;
                float u_span=bw*100.0f/a.zoom_pct;
                float v_span=bh*100.0f/a.zoom_pct;
                float u0=(1.0f-u_span)*(100-a.desloc_x)/200.0f;
                float v0=(1.0f-v_span)*(100-a.desloc_y)/200.0f;
                rounded(r,capa,arte,raio-borda,
                        (SDL_Color){luz,luz,luz,255},
                        u0,v0,u0+u_span,v0+v_span);
            }
            return;
        }
    }
    rounded(r,NULL,arte,raio-borda,(SDL_Color){51,72,106,255},0,0,1,1);
    char curto[256];
    ui_texto_ajustar(titulo?titulo:"Jogo",arte.w-22,1,curto,sizeof(curto));
    ui_texto_desenhar_centro(curto,arte.x+arte.w/2,
                             arte.y+arte.h/2-ui_texto_altura(1)/2,1,
                             (SDL_Color){246,249,255,255},
                             (SDL_Color){0,0,0,100});
}
