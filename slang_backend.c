#include "slang_backend.h"
#include <SDL2/SDL.h>
#include <string.h>
#define LIBRA_RUNTIME_OPENGL
#include "third_party/librashader/librashader.h"

#define FOFITA_GL_RGBA8 0x8058

static struct {
    void *lib;
    PFN_libra_instance_abi_version abi_version;
    PFN_libra_instance_api_version api_version;
    PFN_libra_error_write error_write;
    PFN_libra_error_free error_free;
    PFN_libra_error_free_string error_free_string;
    PFN_libra_preset_create preset_create;
    PFN_libra_preset_free preset_free;
    PFN_libra_gl_filter_chain_create chain_create;
    PFN_libra_gl_filter_chain_frame chain_frame;
    PFN_libra_gl_filter_chain_free chain_free;
} api;
static libra_gl_filter_chain_t cadeia;

/* Converte e libera um erro do librashader. Retorna 1 se NAO houve erro. */
static int conferir(libra_error_t e, const char *etapa, char *erro, size_t n) {
    if (!e) return 1;
    char *texto = NULL;
    if (api.error_write(e, &texto) == 0 && texto) {
        SDL_snprintf(erro, n, "%s: %s", etapa, texto);
        api.error_free_string(&texto);
    } else SDL_snprintf(erro, n, "%s: erro desconhecido do librashader", etapa);
    api.error_free(&e);
    return 0;
}

#define SIMBOLO(campo, tipo, nome) \
    do { \
        api.campo = (tipo)SDL_LoadFunction(api.lib, nome); \
        if (!api.campo) { \
            SDL_snprintf(erro, n, "simbolo ausente na biblioteca: %s", nome); \
            goto falha; \
        } \
    } while (0)

int slang_biblioteca_carregar(const char *caminho, char *erro, size_t n) {
    /* A biblioteca permanece carregada ate o fim do processo: descarregar
       DLLs com runtime proprio a cada jogo nao traz beneficio. */
    if (api.lib) return 1;
    if (!caminho || !*caminho) {
        SDL_snprintf(erro, n, "caminho da biblioteca vazio");
        return 0;
    }
    api.lib = SDL_LoadObject(caminho);
    if (!api.lib) {
        SDL_snprintf(erro, n, "biblioteca indisponivel: %s", SDL_GetError());
        return 0;
    }
    SIMBOLO(abi_version, PFN_libra_instance_abi_version, "libra_instance_abi_version");
    SIMBOLO(api_version, PFN_libra_instance_api_version, "libra_instance_api_version");
    /* Verificar compatibilidade antes de resolver ou chamar o restante. */
    if (api.abi_version() != LIBRASHADER_CURRENT_ABI ||
        api.api_version() < LIBRASHADER_CURRENT_VERSION) {
        SDL_snprintf(erro, n, "librashader incompativel (ABI %u, API %u)",
                     (unsigned)api.abi_version(), (unsigned)api.api_version());
        goto falha;
    }
    SIMBOLO(error_write, PFN_libra_error_write, "libra_error_write");
    SIMBOLO(error_free, PFN_libra_error_free, "libra_error_free");
    SIMBOLO(error_free_string, PFN_libra_error_free_string, "libra_error_free_string");
    SIMBOLO(preset_create, PFN_libra_preset_create, "libra_preset_create");
    SIMBOLO(preset_free, PFN_libra_preset_free, "libra_preset_free");
    SIMBOLO(chain_create, PFN_libra_gl_filter_chain_create, "libra_gl_filter_chain_create");
    SIMBOLO(chain_frame, PFN_libra_gl_filter_chain_frame, "libra_gl_filter_chain_frame");
    SIMBOLO(chain_free, PFN_libra_gl_filter_chain_free, "libra_gl_filter_chain_free");
    return 1;
falha:
    slang_biblioteca_descarregar();
    return 0;
}

void slang_biblioteca_descarregar(void) {
    if (api.lib) SDL_UnloadObject(api.lib);
    memset(&api, 0, sizeof(api));
    cadeia = NULL;
}

static const void *carregar_gl(const char *nome) {
    return SDL_GL_GetProcAddress(nome);
}

int slang_cadeia_criar(const char *preset_caminho, char *erro, size_t n) {
    if (!api.lib) {
        SDL_snprintf(erro, n, "biblioteca nao carregada");
        return 0;
    }
    slang_cadeia_destruir();
    libra_shader_preset_t preset = NULL;
    if (!conferir(api.preset_create(preset_caminho, &preset), "preset", erro, n))
        return 0;
    struct filter_chain_gl_opt_t opcoes;
    memset(&opcoes, 0, sizeof(opcoes));
    opcoes.version = LIBRASHADER_CURRENT_VERSION;
    opcoes.glsl_version = 330;
    opcoes.use_dsa = false;
    opcoes.disable_cache = true; /* nao gravar cache em disco */
    /* A criacao consome o preset, inclusive quando falha. */
    int ok = conferir(api.chain_create(&preset, carregar_gl, &opcoes, &cadeia),
                      "compilacao do shader", erro, n);
    if (preset) {
        libra_error_t e = api.preset_free(&preset);
        if (e) api.error_free(&e);
    }
    if (!ok) cadeia = NULL;
    return ok && cadeia != NULL;
}

int slang_cadeia_quadro(uint32_t entrada, int ew, int eh,
                        uint32_t saida, int sw, int sh,
                        size_t contador, int limpar_historico,
                        char *erro, size_t n) {
    if (!cadeia) {
        SDL_snprintf(erro, n, "cadeia de shader inexistente");
        return 0;
    }
    struct libra_image_gl_t img = {entrada, FOFITA_GL_RGBA8, (uint32_t)ew, (uint32_t)eh};
    struct libra_image_gl_t out = {saida, FOFITA_GL_RGBA8, (uint32_t)sw, (uint32_t)sh};
    struct frame_gl_opt_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.version = LIBRASHADER_CURRENT_VERSION;
    opt.clear_history = limpar_historico != 0;
    opt.frame_direction = 1;
    opt.total_subframes = 1;
    opt.current_subframe = 1;
    opt.frames_per_second = 60.0f;
    opt.brightness_nits = 200.0f;
    return conferir(api.chain_frame(&cadeia, contador, img, out, NULL, NULL, &opt),
                    "renderizacao do shader", erro, n);
}

void slang_cadeia_destruir(void) {
    if (cadeia && api.lib) {
        libra_error_t e = api.chain_free(&cadeia);
        if (e) api.error_free(&e);
    }
    cadeia = NULL;
}
