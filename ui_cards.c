#include "ui_cards.h"
#include "ui_text.h"
#include <math.h>
#include <stdio.h>

/* Card digital quadrado, inspirado na referencia. O foco e o catalogo
   continuam pertencendo ao carrossel; aqui so desenhamos as capas. */
#define CANTOS_PASSOS 7
#define VERTICES_MAX (1 + 4*(CANTOS_PASSOS+1))
#define INDICES_MAX (4*(CANTOS_PASSOS+1)*3)

/* Poligono de cantos arredondados com UV ajustado para crop central.
   SDL_RenderGeometry evita duplicar texturas e nao carrega novas capas. */
static void rounded(SDL_Renderer *r, SDL_Texture *imagem, SDL_Rect rect,
                    int raio, SDL_Color tinta,
                    float u0, float v0, float u1, float v1) {
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
            float tu=u0+(px-rect.x)/rect.w*(u1-u0);
            float tv=v0+(py-rect.y)/rect.h*(v1-v0);
            vertices[n++]=(SDL_Vertex){{px,py},tinta,{tu,tv}};
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

void ui_cards_desenhar(SDL_Renderer *r, SDL_Texture *capa,
                       const char *titulo, SDL_Rect area,
                       int intensidade, int selecionado) {
    if(!r || area.w<=0 || area.h<=0) return;
    int lado=area.w<area.h?area.w:area.h;
    SDL_Rect carta={area.x+(area.w-lado)/2,area.y+(area.h-lado)/2,lado,lado};
    int raio=lado/9;
    if(raio<6) raio=6;
    int borda=selecionado?4:2;
    /* Sombra discreta sob o card selecionado. */
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
            /* Preenche o quadrado sem deformar a imagem; crop central. */
            float u0=0,v0=0,u1=1,v1=1;
            if(tw>th) {
                float frac=(float)th/tw;
                u0=(1.0f-frac)*0.5f;
                u1=u0+frac;
            } else if(th>tw) {
                float frac=(float)tw/th;
                v0=(1.0f-frac)*0.5f;
                v1=v0+frac;
            }
            Uint8 luz=(Uint8)(intensidade<0?0:intensidade>255?255:intensidade);
            rounded(r,capa,arte,raio-borda>0?raio-borda:1,
                    (SDL_Color){luz,luz,luz,255},u0,v0,u1,v1);
            return;
        }
    }
    /* Placeholder enquanto capas sao adicionadas ao catalogo. */
    rounded(r,NULL,arte,raio-borda>0?raio-borda:1,
            (SDL_Color){51,72,106,255},0,0,1,1);
    char curto[256];
    ui_texto_ajustar(titulo?titulo:"Jogo",arte.w-22,1,curto,sizeof(curto));
    ui_texto_desenhar_centro(curto,arte.x+arte.w/2,
                             arte.y+arte.h/2-ui_texto_altura(1)/2,1,
                             (SDL_Color){246,249,255,255},
                             (SDL_Color){0,0,0,100});
}
