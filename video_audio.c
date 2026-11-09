#include "video_audio.h"
#include <SDL2/SDL_image.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *textura_jogo = NULL;
SDL_AudioDeviceID dispositivo_audio;

int formato_de_cor = SDL_PIXELFORMAT_RGB555;
float proporcao_video = 0.0f;

bool init_video_audio(void) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_AudioSpec esp_audio = {0};
    esp_audio.freq = 44100;
    esp_audio.format = AUDIO_S16SYS;
    esp_audio.channels = 2;
    esp_audio.samples = 1024;

    dispositivo_audio = SDL_OpenAudioDevice(NULL, 0, &esp_audio, NULL, 0);
    SDL_PauseAudioDevice(dispositivo_audio, 0);

    window = SDL_CreateWindow("Fofita", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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