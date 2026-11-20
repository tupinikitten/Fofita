#ifndef FOFITA_STATE_STORE_H
#define FOFITA_STATE_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FOFITA_STATE_SLOTS 3u
#define FOFITA_STATE_MAX_BYTES (64u * 1024u * 1024u)
#define FOFITA_STATE_PATH_MAX 2048

typedef enum {
    STATE_IO_OK = 0,
    STATE_IO_EMPTY,
    STATE_IO_UNSUPPORTED,
    STATE_IO_INCOMPATIBLE,
    STATE_IO_CORRUPT,
    STATE_IO_ERROR
} StateIOResult;

/* Uma sessao representa um par de arquivos (core + ROM) especificos.
   Nao deve ser compartilhada entre jogos. */
typedef struct {
    char directory[FOFITA_STATE_PATH_MAX];
    char game_stem[64];
    uint64_t path_hash;
    uint64_t core_hash;
    uint64_t rom_hash;
    bool active;
} StateStore;

/* state_root e distinto de saves/ da SRAM. Calcula fingerprints dos
   bytes do core e da ROM para impedir carregar estado de arquivos trocados. */
bool state_store_init(StateStore *store, const char *state_root,
                      const char *core_path, const char *rom_path);
void state_store_clear(StateStore *store);
bool state_store_slot_path(const StateStore *store, unsigned slot,
                           char *out, size_t cap);
StateIOResult state_store_write(const StateStore *store, unsigned slot,
                               const void *bytes, size_t size);
StateIOResult state_store_read(const StateStore *store, unsigned slot,
                              void *bytes, size_t expected_size);
const char *state_io_description(StateIOResult result);

#endif
