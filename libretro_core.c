#include "libretro_core.h"
#include "video_audio.h" // Usamos isto para ligar o áudio e o vídeo
#include "libretro.h"
#include "core_content.h"
#include "core_paths.h"
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

/* Mantem o buffer da ROM e o caminho validos enquanto o core esta aberto.
   Cores need_fullpath recebem apenas um caminho, nunca um buffer completo. */
static CoreContent conteudo;
static bool precisa_caminho_completo = false;
static bool core_inicializado = false;
static void (*api_get_system_info)(struct retro_system_info*) = NULL;
static int jogo_carregado = 0;

/* Funcoes opcionais: nao tornam invalido um core que nao as exporta.
   Os recursos reais sao consultados somente depois de carregar o jogo. */
static size_t (*api_serialize_size)(void) = NULL;
static bool (*api_serialize)(void*,size_t) = NULL;
static bool (*api_unserialize)(const void*,size_t) = NULL;
static void *(*api_get_memory_data)(unsigned) = NULL;
static size_t (*api_get_memory_size)(unsigned) = NULL;
static void (*api_cheat_reset)(void) = NULL;
static void (*api_cheat_set)(unsigned,bool,const char*) = NULL;
static CoreCapabilities capacidades;

const CoreCapabilities *core_get_capabilities(void) {
    return &capacidades;
}

static void atualizar_capacidades(void) {
    capacidades = (CoreCapabilities){0};
    if(!jogo_carregado) return;
    if(api_serialize_size && api_serialize && api_unserialize) {
        size_t size = api_serialize_size();
        if(size > 0) {
            capacidades.save_states_available = true;
            capacidades.save_state_bytes = size;
        }
    }
    if(api_get_memory_data && api_get_memory_size) {
        size_t size = api_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        if(size > 0 && api_get_memory_data(RETRO_MEMORY_SAVE_RAM)) {
            capacidades.save_ram_available = true;
            capacidades.save_ram_bytes = size;
        }
    }
    /* Apenas indica a presenca dos callbacks, nao suporte a cada codigo. */
    capacidades.cheat_api_present = api_cheat_reset && api_cheat_set;
}

extern void retro_input_poll_cb(void);
extern int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id);

bool retro_environment_cb(unsigned cmd, void *data) {
    /* Gambatte exige confirmar que o frontend aceita reutilizar o quadro
       anterior. video_audio.c ja preserva a textura quando data == NULL.
       Sem esta resposta o Gambatte rejeita TODAS as ROMs na inicializacao. */
    if(cmd == RETRO_ENVIRONMENT_GET_CAN_DUPE) {
        if(!data) return false;
        *(bool*)data = true;
        return true;
    }
    if(cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY) {
        if(!data) return false;
        const char *dir=core_paths_system_dir();
        if(!dir) return false;
        *(const char**)data=dir;
        return true;
    }
    if(cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
        if(!data) return false;
        const char *dir=core_paths_save_dir();
        if(!dir) return false;
        *(const char**)data=dir;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_GEOMETRY) {
        const struct retro_game_geometry *geometria =
            (const struct retro_game_geometry *)data;
        if (!geometria) return false;
        proporcao_video = geometria->aspect_ratio > 0.0f
            ? geometria->aspect_ratio
            : (geometria->base_height > 0
                ? (float)geometria->base_width / (float)geometria->base_height
                : 0.0f);
        return true;
    }

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
    if(!core_path || !core_path[0]) return false;
    if(core_dll) close_core();
    capacidades = (CoreCapabilities){0};
    precisa_caminho_completo = false;
    core_inicializado = false;
    core_paths_clear();
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
    api_get_system_info = (void (*)(struct retro_system_info*))GetProcAddress(
        core_dll, "retro_get_system_info");

    if (!core_set_environment || !core_set_video || !core_set_audio || !core_set_audio_batch ||
        !core_set_input_poll || !core_set_input_state || !core_init || !core_load_game ||
        !core_run_ptr || !core_deinit || !api_get_system_info) {
        printf("Erro: %s nao parece ser um nucleo libretro valido.\n", core_path);
        FreeLibrary(core_dll);
        core_dll = NULL;
        return false;
    }

    /* SDL_GetPrefPath cria uma pasta gravavel por usuario; o core recebe
       system/ (BIOS) e saves/ independentes do diretorio da instalacao. */
    char *pref_path=SDL_GetPrefPath("TupiniKitten","Fofita");
    bool caminhos_prontos=pref_path && core_paths_init(pref_path);
    SDL_free(pref_path);
    if(!caminhos_prontos) {
        fprintf(stderr,"Erro: sem diretorio persistente para BIOS e saves.\n");
        FreeLibrary(core_dll);
        core_dll=NULL;
        return false;
    }

    /* Consultar metadados antes de retro_init() conforme API Libretro. */
    struct retro_system_info info={0};
    api_get_system_info(&info);
    precisa_caminho_completo=info.need_fullpath;

    /* Carregar os simbolos opcionais depois da validacao dos obrigatorios. */
    api_serialize_size = (size_t (*)(void))GetProcAddress(core_dll,"retro_serialize_size");
    api_serialize = (bool (*)(void*,size_t))GetProcAddress(core_dll,"retro_serialize");
    api_unserialize = (bool (*)(const void*,size_t))GetProcAddress(core_dll,"retro_unserialize");
    api_get_memory_data = (void *(*)(unsigned))GetProcAddress(core_dll,"retro_get_memory_data");
    api_get_memory_size = (size_t (*)(unsigned))GetProcAddress(core_dll,"retro_get_memory_size");
    api_cheat_reset = (void (*)(void))GetProcAddress(core_dll,"retro_cheat_reset");
    api_cheat_set = (void (*)(unsigned,bool,const char*))GetProcAddress(core_dll,"retro_cheat_set");

    core_set_environment(retro_environment_cb);
    core_set_video(retro_video_refresh_cb);
    core_set_audio(retro_audio_sample_cb);
    core_set_audio_batch(retro_audio_sample_batch_cb);
    core_set_input_poll(retro_input_poll_cb);
    core_set_input_state(retro_input_state_cb);

    core_init();
    core_inicializado = true;
    return true;
}

bool load_game(const char* game_path) {
    if(!core_dll || !core_inicializado || !core_load_game ||
       jogo_carregado || !game_path || !game_path[0])
        return false;

    if(!core_content_open(&conteudo,game_path,precisa_caminho_completo)) {
        fprintf(stderr,"Nao foi possivel abrir o arquivo do jogo.\n");
        return false;
    }

    struct retro_game_info info={0};
    info.path=conteudo.path;
    info.data=conteudo.data;
    info.size=conteudo.size;
    if(!core_load_game(&info)) {
        core_content_release(&conteudo);
        fprintf(stderr,"O nucleo recusou a ROM.\n");
        return false;
    }

    jogo_carregado = 1;
    atualizar_capacidades();
    return true;
}

void run_core(void) {
    if (core_run_ptr) core_run_ptr();
}

/* Descarrega o jogo atual (se houver) e libera a memoria da ROM. */
void unload_game(void) {
    if (jogo_carregado && core_unload_game) core_unload_game();
    jogo_carregado = 0;
    capacidades = (CoreCapabilities){0};
    core_content_release(&conteudo);
}

void close_core(void) {
    unload_game();
    if (core_inicializado && core_deinit && core_dll) core_deinit();
    if (core_dll) FreeLibrary(core_dll);
    core_dll = NULL;
    core_inicializado = false;
    precisa_caminho_completo = false;
    api_get_system_info = NULL;
    core_paths_clear();
    core_run_ptr = NULL;
    core_deinit = NULL;
    core_init = NULL;
    core_load_game = NULL;
    core_unload_game = NULL;
    core_set_environment = NULL;
    core_set_video = NULL;
    core_set_audio = NULL;
    core_set_audio_batch = NULL;
    core_set_input_poll = NULL;
    core_set_input_state = NULL;
    api_serialize_size = NULL;
    api_serialize = NULL;
    api_unserialize = NULL;
    api_get_memory_data = NULL;
    api_get_memory_size = NULL;
    api_cheat_reset = NULL;
    api_cheat_set = NULL;
    capacidades = (CoreCapabilities){0};
}
