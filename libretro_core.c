#include "libretro_core.h"
#include "video_audio.h" // Usamos isto para ligar o áudio e o vídeo
#include "libretro.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void (*core_set_environment)(retro_environment_t);
void (*core_set_video)(retro_video_refresh_t);
void (*core_set_audio)(retro_audio_sample_t);
void (*core_set_audio_batch)(retro_audio_sample_batch_t);
void (*core_set_input_poll)(retro_input_poll_t);
void (*core_set_input_state)(retro_input_state_t);
void (*core_init)(void);
bool (*core_load_game)(const struct retro_game_info*);
void (*core_unload_game)(void);
void (*core_run_ptr)(void);
void (*core_deinit)(void);

HMODULE core_dll = NULL;

/* Os dados da ROM precisam continuar validos enquanto o jogo estiver carregado. */
static void *rom_data = NULL;
static int jogo_carregado = 0;

extern void retro_input_poll_cb(void);
extern int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id);

bool retro_environment_cb(unsigned cmd, void *data) { 
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
        enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
        if (*fmt == RETRO_PIXEL_FORMAT_XRGB8888) { formato_de_cor = SDL_PIXELFORMAT_ARGB8888; return true; }
        else if (*fmt == RETRO_PIXEL_FORMAT_RGB565) { formato_de_cor = SDL_PIXELFORMAT_RGB565; return true; }
        else if (*fmt == RETRO_PIXEL_FORMAT_0RGB1555) { formato_de_cor = SDL_PIXELFORMAT_RGB555; return true; }
        return false;
    }
    return false; 
}

bool init_core(const char* core_path) {
    core_dll = LoadLibraryA(core_path);
    if (!core_dll) {
        printf("Erro ao carregar a DLL do nucleo.\n");
        return false;
    }

    core_set_environment = (void (*)(retro_environment_t))GetProcAddress(core_dll, "retro_set_environment");
    core_set_video = (void (*)(retro_video_refresh_t))GetProcAddress(core_dll, "retro_set_video_refresh");
    core_set_audio = (void (*)(retro_audio_sample_t))GetProcAddress(core_dll, "retro_set_audio_sample");
    core_set_audio_batch = (void (*)(retro_audio_sample_batch_t))GetProcAddress(core_dll, "retro_set_audio_sample_batch");
    core_set_input_poll = (void (*)(retro_input_poll_t))GetProcAddress(core_dll, "retro_set_input_poll");
    core_set_input_state = (void (*)(retro_input_state_t))GetProcAddress(core_dll, "retro_set_input_state");
    core_init = (void (*)(void))GetProcAddress(core_dll, "retro_init");
    core_load_game = (bool (*)(const struct retro_game_info*))GetProcAddress(core_dll, "retro_load_game");
    core_unload_game = (void (*)(void))GetProcAddress(core_dll, "retro_unload_game");
    core_run_ptr = (void (*)(void))GetProcAddress(core_dll, "retro_run");
    core_deinit = (void (*)(void))GetProcAddress(core_dll, "retro_deinit");

    if (!core_set_environment || !core_set_video || !core_set_audio || !core_set_audio_batch ||
        !core_set_input_poll || !core_set_input_state || !core_init || !core_load_game ||
        !core_run_ptr || !core_deinit) {
        printf("Erro: %s nao parece ser um nucleo libretro valido.\n", core_path);
        FreeLibrary(core_dll);
        core_dll = NULL;
        return false;
    }

    core_set_environment(retro_environment_cb);
    core_set_video(retro_video_refresh_cb);
    core_set_audio(retro_audio_sample_cb);
    core_set_audio_batch(retro_audio_sample_batch_cb);
    core_set_input_poll(retro_input_poll_cb);
    core_set_input_state(retro_input_state_cb);

    core_init();
    return true;
}

bool load_game(const char* game_path) {
    struct retro_game_info info = {0};
    info.path = game_path;

    FILE *file = fopen(info.path, "rb");
    if (!file) {
        printf("Nao foi possivel encontrar a ROM!\n");
        return false;
    }

    fseek(file, 0, SEEK_END);
    long tamanho = ftell(file);
    rewind(file);

    if (tamanho <= 0) {
        fclose(file);
        printf("A ROM esta vazia ou ilegivel.\n");
        return false;
    }

    rom_data = malloc((size_t)tamanho);
    if (!rom_data) {
        fclose(file);
        printf("Memoria insuficiente para carregar a ROM.\n");
        return false;
    }

    size_t lidos = fread(rom_data, 1, (size_t)tamanho, file);
    fclose(file);

    if (lidos != (size_t)tamanho) {
        free(rom_data);
        rom_data = NULL;
        printf("Falha ao ler a ROM.\n");
        return false;
    }

    info.data = rom_data;
    info.size = (size_t)tamanho;

    if (!core_load_game(&info)) {
        free(rom_data);
        rom_data = NULL;
        printf("O nucleo recusou a ROM.\n");
        return false;
    }

    jogo_carregado = 1;
    return true;
}

void run_core(void) {
    if (core_run_ptr) core_run_ptr();
}

/* Descarrega o jogo atual (se houver) e libera a memoria da ROM. */
void unload_game(void) {
    if (jogo_carregado && core_unload_game) core_unload_game();
    jogo_carregado = 0;
    free(rom_data);
    rom_data = NULL;
}

void close_core(void) {
    unload_game();
    if (core_deinit && core_dll) core_deinit();
    if (core_dll) FreeLibrary(core_dll);
    core_dll = NULL;
    core_run_ptr = NULL;
    core_deinit = NULL;
}
