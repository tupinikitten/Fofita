#include "video_audio.h"
#include <SDL2/SDL_image.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *textura_jogo = NULL;
SDL_AudioDeviceID dispositivo_audio;
static SDL_AudioDeviceID dispositivo_ui = 0;
static Uint8 *sons_ui[2] = {NULL,NULL};
static Uint32 tamanhos_ui[2] = {0,0};
static Uint32 ultimo_som_navegacao = 0;

static void carregar_sons_ui(void) {
    const char *arquivos[2] = {
        "ui/sounds/ui-menu-seletion.wav",
        "ui/sounds/switch-button-sound.wav"
    };
    SDL_AudioSpec formato = {0};
    formato.freq = 44100;
    formato.format = AUDIO_S16SYS;
    formato.channels = 2;
    formato.samples = 1024;
    dispositivo_ui = SDL_OpenAudioDevice(NULL, 0, &formato, NULL, 0);
    if (!dispositivo_ui) return;
    SDL_PauseAudioDevice(dispositivo_ui, 0);
    for (int i=0;i<2;++i) {
        SDL_AudioSpec origem;
        Uint8 *dados = NULL;
        Uint32 tamanho = 0;
        if (!SDL_LoadWAV(arquivos[i], &origem, &dados, &tamanho)) {
            SDL_Log("Som UI indisponivel: %s", arquivos[i]);
            continue;
        }
        SDL_AudioCVT conversor;
        if (SDL_BuildAudioCVT(&conversor, origem.format, origem.channels,
                             origem.freq, formato.format, formato.channels,
                             formato.freq) < 0) {
            SDL_FreeWAV(dados);
            continue;
        }
        conversor.len = (int)tamanho;
        conversor.buf = (Uint8 *)SDL_malloc(tamanho * conversor.len_mult);
        if (conversor.buf) {
            SDL_memcpy(conversor.buf,dados,tamanho);
            if (SDL_ConvertAudio(&conversor)==0) {
                sons_ui[i]=conversor.buf;
                tamanhos_ui[i]=(Uint32)conversor.len_cvt;
                if(i==UI_SOM_NAVEGACAO) {
                    /* Cliques curtos e sem estalos: a tecla pode repetir
                       continuamente sem cortar o sample anterior. */
                    Uint32 maximo=44100*4*110/1000;
                    if(tamanhos_ui[i]>maximo) tamanhos_ui[i]=maximo;
                    Uint32 quadros=tamanhos_ui[i]/4;
                    Uint32 fade=quadros<441?quadros:441;
                    Sint16 *pcm=(Sint16 *)sons_ui[i];
                    for(Uint32 j=0;j<fade;++j) {
                        float ganho=(float)(fade-j)/(float)fade;
                        Uint32 k=(quadros-fade+j)*2;
                        pcm[k]=(Sint16)(pcm[k]*ganho);
                        pcm[k+1]=(Sint16)(pcm[k+1]*ganho);
                    }
                }
            } else SDL_free(conversor.buf);
        }
        SDL_FreeWAV(dados);
    }
}
void ui_som_tocar(UiSom som) {
    if (!dispositivo_ui || som<UI_SOM_NAVEGACAO || som>UI_SOM_TROCA ||
        !sons_ui[som] || !tamanhos_ui[som]) return;
    if(som==UI_SOM_NAVEGACAO) {
        Uint32 agora=SDL_GetTicks();
        if(ultimo_som_navegacao && agora-ultimo_som_navegacao<88) return;
        /* Nao reiniciar o clique a cada tecla repetida: deixa soar ate
           o final; limita a fila para nao atrasar o som da selecao. */
        if(SDL_GetQueuedAudioSize(dispositivo_ui)>44100*4*55/1000) return;
        ultimo_som_navegacao=agora;
    } else {
        SDL_ClearQueuedAudio(dispositivo_ui);
    }
    SDL_QueueAudio(dispositivo_ui,sons_ui[som],tamanhos_ui[som]);
}


int formato_de_cor = SDL_PIXELFORMAT_RGB555;
float proporcao_video = 0.0f;

static int modo_menu_ativo = 0;

/* O dashboard usa as coordenadas historicas 1280x720.
   SDL_RenderSetLogicalSize calcula uma unica escala uniforme para qualquer
   janela (720p, 1080p ou redimensionada), sem alterar o renderer manualmente.
   Remover a escala extra evita desenhar apenas 2/3 da janela em 1080p. */
void configurar_video_menu(void) {
    if (!renderer) return;
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetLogicalSize(renderer, FOFITA_UI_W, FOFITA_UI_H);
    modo_menu_ativo=1;
}

void configurar_video_jogo(void) {
    if(dispositivo_ui) SDL_ClearQueuedAudio(dispositivo_ui);
    if (!renderer) return;
    modo_menu_ativo=0;
    SDL_RenderSetLogicalSize(renderer,0,0);
    SDL_RenderSetScale(renderer,1.0f,1.0f);
    SDL_RenderSetViewport(renderer,NULL);
}

void atualizar_video_redimensionado(void) {
    if(modo_menu_ativo) configurar_video_menu();
}

void obter_tamanho_ui(int *w,int *h) {
    if(w) *w=FOFITA_UI_W;
    if(h) *h=FOFITA_UI_H;
}

bool init_video_audio(void) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    /* Suaviza o reescalonamento de PNGs e demais texturas da UI. */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    SDL_AudioSpec esp_audio = {0};
    esp_audio.freq = 44100;
    esp_audio.format = AUDIO_S16SYS;
    esp_audio.channels = 2;
    esp_audio.samples = 1024;

    dispositivo_audio = SDL_OpenAudioDevice(NULL, 0, &esp_audio, NULL, 0);
    SDL_PauseAudioDevice(dispositivo_audio, 0);
    carregar_sons_ui();

    window = SDL_CreateWindow("Fofita", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer) return false;
    configurar_video_menu();
    return true;
}

void fechar_video_audio(void) {
    if (dispositivo_ui) {
        SDL_CloseAudioDevice(dispositivo_ui);
        dispositivo_ui=0;
    }
    for(int i=0;i<2;++i) {
        SDL_free(sons_ui[i]);
        sons_ui[i]=NULL;
        tamanhos_ui[i]=0;
    }
    if (dispositivo_audio) {
        SDL_CloseAudioDevice(dispositivo_audio);
        dispositivo_audio=0;
    }
    if (textura_jogo) {
        SDL_DestroyTexture(textura_jogo);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void descartar_textura_jogo(void) {
    if (textura_jogo) {
        SDL_DestroyTexture(textura_jogo);
        textura_jogo = NULL;
    }
}

void limpar_audio(void) {
    if (dispositivo_audio) SDL_ClearQueuedAudio(dispositivo_audio);
}

void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (data) {
        int textura_w = 0, textura_h = 0;
        if (textura_jogo)
            SDL_QueryTexture(textura_jogo, NULL, NULL, &textura_w, &textura_h);

        /* Alguns nucleos podem alterar a resolucao durante a execucao. */
        if (!textura_jogo || textura_w != (int)width || textura_h != (int)height) {
            descartar_textura_jogo();
            textura_jogo = SDL_CreateTexture(renderer, formato_de_cor,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             (int)width, (int)height);
            if (!textura_jogo) return;
        }
        SDL_UpdateTexture(textura_jogo, NULL, data, (int)pitch);
    }
}

void retro_audio_sample_cb(int16_t left, int16_t right) {
    int16_t frame[2] = {left, right};
    SDL_QueueAudio(dispositivo_audio, frame, sizeof(frame));
}

size_t retro_audio_sample_batch_cb(const int16_t *data, size_t frames) {
    SDL_QueueAudio(dispositivo_audio, data, frames * 2 * sizeof(int16_t));
    return frames;
}