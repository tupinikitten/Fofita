#include "ui_text.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>
/* Atlas prerenderizado da fonte Lato Semibold: nao requer SDL_ttf. */
#define CELL 40
static SDL_Texture *atlas = NULL;
static SDL_Renderer *r = NULL;
static const unsigned char larguras[224] = {6, 7, 9, 14, 14, 19, 17, 6, 6, 6, 10, 14, 6, 9, 6, 11, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 6, 6, 14, 14, 14, 11, 20, 16, 16, 16, 18, 14, 14, 17, 18, 7, 10, 16, 12, 22, 18, 19, 15, 19, 15, 13, 14, 18, 16, 25, 16, 15, 14, 7, 11, 7, 14, 11, 10, 12, 14, 12, 14, 13, 9, 13, 13, 6, 6, 13, 6, 20, 13, 14, 14, 14, 9, 10, 9, 13, 13, 19, 12, 13, 11, 7, 6, 7, 14, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 6, 6, 14, 14, 14, 14, 6, 12, 10, 20, 9, 11, 14, 6, 20, 10, 10, 14, 8, 8, 10, 16, 17, 6, 10, 8, 10, 11, 18, 18, 18, 11, 16, 16, 16, 16, 16, 16, 22, 16, 14, 14, 14, 14, 7, 7, 7, 7, 18, 18, 19, 19, 19, 19, 19, 14, 19, 18, 18, 18, 18, 15, 15, 14, 12, 12, 12, 12, 12, 12, 19, 12, 13, 13, 13, 13, 6, 6, 6, 6, 14, 13, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 14, 13};
static int prox(const unsigned char **ptr) {
    const unsigned char *s=*ptr;
    if (!*s) return -1;
    unsigned c=*s++;
    if (c>=0xC2 && c<=0xC3 && (*s&0xC0)==0x80) {
        c=((c&31)<<6)|(*s++&63);
    } else if(c>=128) {
        while((*s&0xC0)==0x80) ++s;
        c='?';
    }
    *ptr=s;
    return (c>=32 && c<=255) ? (int)c-32 : ('?'-32);
}
static float fator(int escala) { return escala<=1 ? 0.67f : (escala==2 ? 1.f : escala*0.5f); }
int ui_texto_init(SDL_Renderer *renderer) {
    ui_texto_fechar();
    if (!renderer) return 0;
    atlas=IMG_LoadTexture(renderer,"ui/font/lato_atlas.png");
    if (!atlas) { fprintf(stderr,"Atlas da fonte nao encontrado: %s\n",IMG_GetError()); return 0; }
    SDL_SetTextureBlendMode(atlas,SDL_BLENDMODE_BLEND);
    r=renderer; return 1;
}
void ui_texto_fechar(void) { if(atlas) SDL_DestroyTexture(atlas); atlas=NULL; r=NULL; }
int ui_texto_altura(int escala) { return (int)(29*fator(escala)+0.5f); }
int ui_texto_largura(const char *texto,int escala) {
    if(!texto) return 0;
    const unsigned char *p=(const unsigned char*)texto; int c; float n=0;
    while((c=prox(&p))>=0) n+=larguras[c]+1;
    return (int)(n*fator(escala)+0.5f);
}
static void texto_draw(const char *texto,int x,int y,int escala,SDL_Color cor) {
    if (!atlas || !texto) return;
    float f=fator(escala); float pos=(float)x;
    SDL_SetTextureColorMod(atlas,cor.r,cor.g,cor.b);
    SDL_SetTextureAlphaMod(atlas,cor.a);
    const unsigned char *p=(const unsigned char*)texto; int c;
    while((c=prox(&p))>=0) {
        SDL_Rect src={ (c%16)*CELL,(c/16)*CELL,CELL,CELL };
        SDL_Rect dst={ (int)(pos+0.5f),y,(int)(CELL*f+0.5f),(int)(CELL*f+0.5f) };
        SDL_RenderCopy(r,atlas,&src,&dst);
        pos+=(larguras[c]+1)*f;
    }
}
void ui_texto_desenhar(const char *texto,int x,int y,int escala,SDL_Color cor,SDL_Color sombra) {
    if(sombra.a) texto_draw(texto,x+1,y+2,escala,sombra);
    texto_draw(texto,x,y,escala,cor);
}
void ui_texto_desenhar_centro(const char *texto,int centro_x,int y,int escala,SDL_Color cor,SDL_Color sombra) {
    ui_texto_desenhar(texto,centro_x-ui_texto_largura(texto,escala)/2,y,escala,cor,sombra);
}
void ui_texto_ajustar(const char *texto,int max_largura,int escala,char *saida,size_t capacidade) {
    if(!saida||!capacidade) return;
    snprintf(saida,capacidade,"%s",texto?texto:"");
    if(ui_texto_largura(saida,escala)<=max_largura) return;
    size_t len=strlen(saida);
    while(len) {
        do { --len; } while(len && ((unsigned char)saida[len]&0xc0)==0x80);
        if(len+4>capacidade) continue;
        memcpy(saida+len,"...",4);
        if(ui_texto_largura(saida,escala)<=max_largura) return;
    }
    snprintf(saida,capacidade,"...");
}
