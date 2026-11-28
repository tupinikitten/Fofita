#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H
#include <SDL2/SDL.h>

typedef enum {
    SHADER_NENHUM = 0,
    SHADER_PASSTHROUGH = 1, /* GLSL 1.20, apenas diagnostico */
    SHADER_SLANG_TESTE = 2, /* preset .slangp executado pelo librashader */
    SHADER_TOTAL
} ShaderModo;

extern int video_shader; /* ShaderModo escolhido nas Configuracoes de video */

/* Passthrough offscreen, na resolucao do jogo. Retorna NULL se desativado ou
   em falha; o chamador deve entao exibir o frame original. */
const void *shader_processar(const void *data, int w, int h, int pitch,
                             Uint32 format, int *out_pitch);

/* Executa o preset Slang e devolve pixels RGBA32 (saida_w*4 bytes por linha),
   de cima para baixo. Mesmo contrato de fallback do passthrough. */
const void *shader_slang_processar(const void *data, int w, int h, int pitch,
                                   Uint32 format, int saida_w, int saida_h);

/* Libera os recursos GPU. A proxima chamada tenta inicializar novamente. */
void shader_fechar(void);

/* 1 se o modo selecionado falhou na ultima tentativa (para avisar no menu). */
int shader_falhou(void);
const char *shader_ultimo_erro(void);
void shader_limpar_falha(void);

/* Substitui os caminhos padrao (NULL = ao lado do executavel). Para testes. */
void shader_slang_definir_caminhos(const char *biblioteca, const char *preset);
#endif
