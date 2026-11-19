#ifndef FOFITA_CORE_CONTENT_H
#define FOFITA_CORE_CONTENT_H

#include <stdbool.h>
#include <stddef.h>

/* Um jogo tem o caminho original (sempre mantido) e, opcionalmente,
   um buffer de ROM. Os ponteiros sao validos ate core_content_release(). */
typedef struct {
    char *path;
    void *data;
    size_t size;
} CoreContent;

/* Exige *out inicialmente zerado; em falha, deixa-o zerado.
   need_fullpath: valida arquivo mas nao o carrega para a RAM. */
bool core_content_open(CoreContent *out, const char *path, bool need_fullpath);
void core_content_release(CoreContent *content);

#endif
