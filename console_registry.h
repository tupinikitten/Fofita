#ifndef CONSOLE_REGISTRY_H
#define CONSOLE_REGISTRY_H

#include <stddef.h>

typedef enum {
    CAPA_STRETCH = 0, /* preenche a etiqueta, podendo deformar */
    CAPA_CONTAIN = 1, /* mostra a imagem inteira, sem deformar */
    CAPA_CROP = 2,    /* preenche a etiqueta, recortando o excesso */
    CAPA_AUTO = 3     /* escolhe sozinho: recorta se a proporcao for parecida com a da
                         etiqueta; senao mostra a arte inteira sobre um fundo suave */
} ModoEncaixeCapa;

typedef struct {
    const char *id;
    const char *nome;
    const char *const *extensoes;
    size_t quantidade_extensoes;
    const char *pasta_roms;
    const char *pasta_capas;
    const char *caminho_molde;
    int molde_w, molde_h;
    int off_x, off_y, label_w, label_h;
    ModoEncaixeCapa modo_encaixe_capa;
    int raio_cantos_inferiores; /* raio da mascara da etiqueta, em pixels do molde */
    /* Campos finais: ficam NULL (zero) quando um console ainda nao os define. */
    const char *caminho_core;   /* DLL do nucleo libretro; NULL = console ainda nao ativo */
    const char *caminho_icone;  /* icone do console no menu; NULL = mostra so o nome */
} ConsoleConfig;

const ConsoleConfig *console_registry_get_all(size_t *quantidade);
const ConsoleConfig *console_registry_find_by_extension(const char *extensao);

/* Console "ativo" = tem nucleo configurado e o arquivo do nucleo existe. */
int console_registry_esta_ativo(const ConsoleConfig *console);

#endif