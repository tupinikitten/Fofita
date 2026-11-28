#include "game_video.h"
#include "shader_manager.h"
#include "video_math.h"
#include "video_audio.h"
#include <stdlib.h>
#include <string.h>

SDL_Texture *textura_jogo = NULL;
int formato_de_cor = SDL_PIXELFORMAT_RGB555;
float proporcao_video = 0.0f;
int video_escala = 0, video_filtro = 0, video_proporcao = 0;

/* Copia do ultimo frame para o Slang, que renderiza na resolucao final
   (o ponteiro do core so e valido durante o callback). */
static unsigned char *quadro;
static size_t quadro_capacidade;
static int quadro_w, quadro_h, quadro_novo;
static Uint32 quadro_formato;
static SDL_Texture *textura_slang;
static int slang_w, slang_h, slang_valida;

static void descartar_saida_slang(void) {
    if (textura_slang) SDL_DestroyTexture(textura_slang);
    textura_slang = NULL;
    slang_w = slang_h = slang_valida = 0;
}

void descartar_textura_jogo(void) {
    shader_fechar();
    descartar_saida_slang();
    free(quadro);
    quadro = NULL;
    quadro_capacidade = 0;
    quadro_w = quadro_h = quadro_novo = 0;
    if (textura_jogo) {
        SDL_DestroyTexture(textura_jogo);
        textura_jogo = NULL;
    }
}

static void guardar_quadro(const void *data, int w, int h, size_t pitch) {
    size_t linha = (size_t)w * SDL_BYTESPERPIXEL((Uint32)formato_de_cor);
    quadro_w = 0; /* invalida ate a copia terminar */
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096 || pitch < linha) return;
    if (linha * h > quadro_capacidade) {
        unsigned char *novo = realloc(quadro, linha * h);
        if (!novo) return;
        quadro = novo;
        quadro_capacidade = linha * h;
    }
    for (int y = 0; y < h; ++y)
        memcpy(quadro + y * linha, (const unsigned char *)data + y * pitch, linha);
    quadro_w = w;
    quadro_h = h;
    quadro_formato = (Uint32)formato_de_cor;
    quadro_novo = 1;
}

void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (data) {
        int textura_w = 0, textura_h = 0;
        if (textura_jogo)
            SDL_QueryTexture(textura_jogo, NULL, NULL, &textura_w, &textura_h);

        /* Alguns nucleos podem alterar a resolucao durante a execucao.
           Os shaders continuam carregados: apenas a textura e recriada. */
        if (!textura_jogo || textura_w != (int)width || textura_h != (int)height) {
            if (textura_jogo) SDL_DestroyTexture(textura_jogo);
            textura_jogo = SDL_CreateTexture(renderer, formato_de_cor,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             (int)width, (int)height);
            if (!textura_jogo) return;
            /* Game textures use nearest-neighbor scaling independently of the
               linear filtering used by PNGs and other interface assets. */
#if SDL_VERSION_ATLEAST(2, 0, 12)
            if (SDL_SetTextureScaleMode(textura_jogo, video_filtro ? SDL_ScaleModeLinear : SDL_ScaleModeNearest) != 0)
                SDL_Log("Nao foi possivel ativar escala nearest: %s", SDL_GetError());
#endif
        }
        if (video_shader == SHADER_SLANG_TESTE && !shader_falhou())
            guardar_quadro(data, (int)width, (int)height, pitch);

        int shader_pitch = 0;
        const void *processed = shader_processar(data, (int)width, (int)height, (int)pitch, formato_de_cor, &shader_pitch);
        /* A imagem original fica sempre atualizada: e o fallback imediato. */
        SDL_UpdateTexture(textura_jogo, NULL, processed ? processed : data, processed ? shader_pitch : (int)pitch);
    }
}

/* Atualiza a saida Slang quando ha frame novo ou o destino muda de tamanho.
   Retorna a textura a desenhar, ou NULL para usar a imagem original. */
static SDL_Texture *aplicar_slang(int destino_w, int destino_h) {
    if (video_shader != SHADER_SLANG_TESTE || shader_falhou() || quadro_w <= 0)
        return NULL;
    int w = destino_w > 4096 ? 4096 : destino_w;
    int h = destino_h > 4096 ? 4096 : destino_h;
    if (!quadro_novo && slang_valida && w == slang_w && h == slang_h)
        return textura_slang;

    const void *pixels = shader_slang_processar(
        quadro, quadro_w, quadro_h,
        quadro_w * (int)SDL_BYTESPERPIXEL(quadro_formato), quadro_formato, w, h);
    quadro_novo = 0;
    if (!pixels) { descartar_saida_slang(); return NULL; }
    if (!textura_slang || w != slang_w || h != slang_h) {
        descartar_saida_slang();
        textura_slang = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!textura_slang) return NULL;
#if SDL_VERSION_ATLEAST(2, 0, 12)
        SDL_SetTextureScaleMode(textura_slang, SDL_ScaleModeNearest);
#endif
        SDL_SetTextureBlendMode(textura_slang, SDL_BLENDMODE_NONE);
        slang_w = w;
        slang_h = h;
    }
    slang_valida = SDL_UpdateTexture(textura_slang, NULL, pixels, w * 4) == 0;
    return slang_valida ? textura_slang : NULL;
}

void game_video_desenhar(const char *console_id) {
    if (!textura_jogo) return;

    int janela_w = 0, janela_h = 0;
    int jogo_w = 0, jogo_h = 0;
    SDL_GetRendererOutputSize(renderer, &janela_w, &janela_h);
    SDL_QueryTexture(textura_jogo, NULL, NULL, &jogo_w, &jogo_h);

    SDL_Rect destino;
    if (!video_calcular_destino(janela_w, janela_h, jogo_w, jogo_h, console_id,
                                proporcao_video, video_proporcao, video_escala,
                                &destino))
        return;

    SDL_Texture *com_shader = aplicar_slang(destino.w, destino.h);
    SDL_RenderCopy(renderer, com_shader ? com_shader : textura_jogo, NULL, &destino);
}
