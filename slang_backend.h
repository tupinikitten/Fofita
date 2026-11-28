#ifndef SLANG_BACKEND_H
#define SLANG_BACKEND_H
#include <stddef.h>
#include <stdint.h>
/* Ponte com a API C do librashader (OpenGL 3.3). A biblioteca e carregada em
   tempo de execucao: sem ela, o Fofita continua funcionando sem shaders.
   Todas as funcoes slang_cadeia_* exigem o contexto GL do chamador atual.
   Em caso de erro retornam 0 e preenchem "erro". */

int slang_biblioteca_carregar(const char *caminho, char *erro, size_t n);
void slang_biblioteca_descarregar(void);

int slang_cadeia_criar(const char *preset, char *erro, size_t n);
/* textura_entrada/textura_saida: GL_RGBA8. Renderiza preenchendo a saida. */
int slang_cadeia_quadro(uint32_t textura_entrada, int ew, int eh,
                        uint32_t textura_saida, int sw, int sh,
                        size_t contador, int limpar_historico,
                        char *erro, size_t n);
void slang_cadeia_destruir(void);
#endif
