#ifndef FOFITA_SAVE_MANAGER_H
#define FOFITA_SAVE_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#define FOFITA_SRAM_MAX_BYTES (32u * 1024u * 1024u)
#define FOFITA_SAVE_PATH_MAX 2048

typedef enum {
    SAVE_RAM_NEW = 0,
    SAVE_RAM_RESTORED,
    SAVE_RAM_BLOCKED
} SaveRamStartResult;

/* Sessao independente da interface. O buffer last_saved guarda o ultimo
   conteudo persistido, evitando regravar a mesma SRAM a cada checkpoint. */
typedef struct {
    char path[FOFITA_SAVE_PATH_MAX];
    unsigned char *last_saved;
    size_t size;
    bool active;
} SaveRamSession;

/* Exige sessao zerada e RAM existente. Se o arquivo estiver corrompido,
   inacessivel ou com tamanho inesperado, nao modifica RAM e BLOQUEIA escrita. */
SaveRamStartResult save_ram_begin(SaveRamSession *session,
                                 const char *save_dir,
                                 const char *core_path,
                                 const char *game_path,
                                 void *ram, size_t size);

/* Grava por arquivo temporario e substituicao atomica, apenas se mudou.
   Retorna false sem modificar o save anterior em caso de falha. */
bool save_ram_checkpoint(SaveRamSession *session,
                         const void *ram, size_t size);

const char *save_ram_path(const SaveRamSession *session);
void save_ram_end(SaveRamSession *session);

#endif
