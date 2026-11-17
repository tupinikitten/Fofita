#include "video_audio.h"
#include <SDL2/SDL_image.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *textura_jogo = NULL;
SDL_AudioDeviceID dispositivo_audio;

int formato_de_cor = SDL_PIXELFORMAT_RGB555;
float proporcao_video = 0.0f;

static int modo_menu_ativo = 0;

/* O canvas e sempre 1920x1080. O desenho existente usa medidas 1280x720,
   convertidas para o canvas SDL pela escala adicional de 1.5. Isso nao
   cria um framebuffer extra: tudo continua indo direto ao renderer ativo. */
void configurar_video_menu(void) {
    if (!renderer) return;
    SDL_RenderSetLogicalSize(renderer, FOFITA_CANVAS_W, FOFITA_CANVAS_H);
    float sx=1.0f, sy=1.0f;
    SDL_RenderGetScale(renderer,&sx,&sy);
    SDL_RenderSetScale(renderer,sx*1.5f,sy*1.5f);
    modo_menu_ativo=1;
}

void configurar_video_jogo(void) {
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

    window = SDL_CreateWindow("Fofita", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer) return false;
    configurar_video_menu();
    return true;
}

void fechar_video_audio(void) {
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