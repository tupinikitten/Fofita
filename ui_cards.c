#include "ui_cards.h"
#include "ui_text.h"

/* Renderizador isolado dos cards. Nada aqui altera foco, ROM ou console. */
static void fill(SDL_Renderer *r, SDL_Rect rect, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_RenderFillRect(r,&rect);
}
static void outline(SDL_Renderer *r, SDL_Rect rect, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_RenderDrawRect(r,&rect);
}
void ui_cards_desenhar(SDL_Renderer *r, SDL_Texture *capa,
                       const char *titulo, SDL_Rect area,
                       int intensidade, int selecionado) {
    if(!r || area.w<=0 || area.h<=0) return;
    /* Formato vertical de capa digital: preserva a mesma caixa do carrossel. */
    int w=(int)(area.w*0.72f);
    int h=(int)(area.h*0.94f);
    if(w<30)w=30;
    if(h<45)h=45;
    SDL_Rect card={area.x+(area.w-w)/2,area.y+(area.h-h)/2,w,h};
    fill(r,(SDL_Rect){card.x+4,card.y+5,card.w,card.h},
         (SDL_Color){9,19,36,selecionado?78:42});
    fill(r,card,(SDL_Color){235,242,249,(Uint8)(selecionado?255:225)});
    outline(r,card,(SDL_Color){46,67,96,(Uint8)(selecionado?245:135)});
    SDL_Rect arte={card.x+8,card.y+8,card.w-16,card.h-16};
    if(capa) {
        int tw=0,th=0;
        SDL_QueryTexture(capa,NULL,NULL,&tw,&th);
        if(tw>0 && th>0) {
            /* Contain: nao distorce capas verticais ou horizontais. */
            float scale=(float)arte.w/tw;
            if((float)arte.h/th<scale) scale=(float)arte.h/th;
            int aw=(int)(tw*scale+0.5f), ah=(int)(th*scale+0.5f);
            SDL_Rect dest={arte.x+(arte.w-aw)/2,arte.y+(arte.h-ah)/2,aw,ah};
            SDL_SetTextureColorMod(capa,(Uint8)intensidade,(Uint8)intensidade,(Uint8)intensidade);
            SDL_RenderCopy(r,capa,NULL,&dest);
            SDL_SetTextureColorMod(capa,255,255,255);
            return;
        }
    }
    fill(r,arte,(SDL_Color){47,72,104,255});
    char curto[256];
    ui_texto_ajustar(titulo?titulo:"Jogo",arte.w-12,1,curto,sizeof(curto));
    ui_texto_desenhar_centro(curto,card.x+card.w/2,
                            card.y+card.h/2-ui_texto_altura(1)/2,1,
                            (SDL_Color){245,249,255,255},
                            (SDL_Color){0,0,0,0});
}
