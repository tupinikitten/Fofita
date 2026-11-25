#include "ui_wallpaper.h"
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <string.h>

/* Limites de memoria para nao sobrecarregar computadores modestos e
   facilitar uma futura adaptacao ao Nintendo Switch. */
#define GIF_LIMITE_QUADROS 96
#define GIF_LIMITE_MEMORIA (64ULL * 1024ULL * 1024ULL)
#define GIF_LIMITE_ARQUIVO (16LL * 1024LL * 1024LL)
#define GIF_LIMITE_LARGURA 1920
#define GIF_LIMITE_ALTURA 1080

typedef struct {
    SDL_Texture *estatica;
    SDL_Texture **quadros;
    Uint32 *atrasos;
    int total;
    int indice;
    Uint32 proxima_troca;
} WallpaperAtivo;

static WallpaperAtivo ativo = {0};

static void limpar(WallpaperAtivo *w) {
    if(w->estatica) SDL_DestroyTexture(w->estatica);
    for(int i=0;i<w->total;++i)
        if(w->quadros && w->quadros[i])
            SDL_DestroyTexture(w->quadros[i]);
    free(w->quadros);
    free(w->atrasos);
    memset(w,0,sizeof(*w));
}

static int eh_gif(const char *caminho) {
    const char *p=strrchr(caminho,'.');
    return p && SDL_strcasecmp(p,".gif")==0;
}

static Uint32 atraso_valido(int ms) {
    if(ms<40) return 40; /* 25 FPS maximo, sem travar o render. */
    if(ms>10000) return 10000;
    return (Uint32)ms;
}

#if defined(SDL_IMAGE_VERSION_ATLEAST)
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
static int tentar_animacao(SDL_Renderer *renderer,const char *caminho,
                          WallpaperAtivo *novo) {
    SDL_RWops *arq=SDL_RWFromFile(caminho,"rb");
    if(!arq) return 0;
    Sint64 tamanho=SDL_RWsize(arq);
    SDL_RWclose(arq);
    if(tamanho<0 || tamanho>GIF_LIMITE_ARQUIVO) return 0;

    IMG_Animation *gif=IMG_LoadAnimation(caminho);
    if(!gif) return 0;
    int valido=gif->count>1 && gif->count<=GIF_LIMITE_QUADROS &&
               gif->w>0 && gif->w<=GIF_LIMITE_LARGURA &&
               gif->h>0 && gif->h<=GIF_LIMITE_ALTURA &&
               gif->frames && gif->delays &&
               (unsigned long long)gif->w*gif->h*4*gif->count<=GIF_LIMITE_MEMORIA;
    if(valido) {
        novo->quadros=calloc((size_t)gif->count,sizeof(*novo->quadros));
        novo->atrasos=calloc((size_t)gif->count,sizeof(*novo->atrasos));
        valido=novo->quadros && novo->atrasos;
    }
    if(valido) {
        novo->total=gif->count;
        for(int i=0;i<gif->count;++i) {
            SDL_Surface *quadro=gif->frames[i];
            if(!quadro || quadro->w<=0 || quadro->h<=0 ||
               quadro->w>GIF_LIMITE_LARGURA || quadro->h>GIF_LIMITE_ALTURA) {
                valido=0;
                break;
            }
            novo->quadros[i]=SDL_CreateTextureFromSurface(renderer,quadro);
            if(!novo->quadros[i]) { valido=0; break; }
            SDL_SetTextureBlendMode(novo->quadros[i],SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
            SDL_SetTextureScaleMode(novo->quadros[i],SDL_ScaleModeLinear);
#endif
            novo->atrasos[i]=atraso_valido(gif->delays[i]);
        }
    }
    IMG_FreeAnimation(gif);
    if(valido) {
        novo->indice=0;
        novo->proxima_troca=SDL_GetTicks()+novo->atrasos[0];
        return 1;
    }
    limpar(novo);
    return 0;
}
#endif
#endif

int ui_wallpaper_carregar(SDL_Renderer *renderer,const char *caminho,
                          int animacoes_ativas) {
    if(!renderer || !caminho || !*caminho) return 0;
    WallpaperAtivo novo={0};
    int carregado=0;
    if(animacoes_ativas && eh_gif(caminho)) {
#if defined(SDL_IMAGE_VERSION_ATLEAST)
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        carregado=tentar_animacao(renderer,caminho,&novo);
#endif
#endif
    }
    if(!carregado) {
        /* GIF de um unico quadro, GIF grande e SDL2_image antigo:
           permanece funcional como imagem estatica. */
        novo.estatica=IMG_LoadTexture(renderer,caminho);
        if(!novo.estatica) return 0;
#if SDL_VERSION_ATLEAST(2,0,12)
        SDL_SetTextureScaleMode(novo.estatica,SDL_ScaleModeLinear);
#endif
    }
    limpar(&ativo);
    ativo=novo;
    return 1;
}

SDL_Texture *ui_wallpaper_textura(void) {
    if(ativo.total<=0 || !ativo.quadros) return ativo.estatica;
    Uint32 agora=SDL_GetTicks();
    if((Sint32)(agora-ativo.proxima_troca)>=0) {
        ativo.indice=(ativo.indice+1)%ativo.total;
        /* Avanca no maximo um quadro por desenho. Nao tenta compensar
           quadros atrasados ao voltar da emulacao ou de uma janela inativa. */
        ativo.proxima_troca=agora+ativo.atrasos[ativo.indice];
    }
    return ativo.quadros[ativo.indice];
}

int ui_wallpaper_esta_animado(void) {
    return ativo.total>1;
}
void ui_wallpaper_fechar(void) { limpar(&ativo); }
