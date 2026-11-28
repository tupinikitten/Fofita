#include "video_config.h"
#include "shader_manager.h"
#include <stdlib.h>
#include <string.h>

void video_config_padroes(VideoPrefs *p) {
    p->escala = p->filtro = p->proporcao = 0;
    p->shader = SHADER_NENHUM;
}

static int numero(const char *texto, int *saida) {
    char *fim = NULL;
    long v = strtol(texto, &fim, 10);
    if (fim == texto || (*fim && *fim != '\r' && *fim != '\n')) return 0;
    if (v < -1000 || v > 1000) return 0;
    *saida = (int)v;
    return 1;
}

int video_config_ler_linha(VideoPrefs *p, const char *linha) {
    int v;
    if (!p || !linha) return 0;
    if (!strncmp(linha, "video_escala=", 13)) {
        if (numero(linha + 13, &v) && v >= 0 && v <= 2) p->escala = v;
    } else if (!strncmp(linha, "video_filtro=", 13)) {
        if (numero(linha + 13, &v) && v >= 0 && v <= 1) p->filtro = v;
    } else if (!strncmp(linha, "video_proporcao=", 16)) {
        /* Valores antigos (ex.: 4:3 forcado = 2) migram para o original. */
        if (numero(linha + 16, &v)) p->proporcao = (v == 1);
    } else if (!strncmp(linha, "video_shader=", 13)) {
        /* Valor desconhecido ou invalido desativa o shader. */
        if (numero(linha + 13, &v))
            p->shader = (v >= 0 && v < SHADER_TOTAL) ? v : SHADER_NENHUM;
        else p->shader = SHADER_NENHUM;
    } else return 0;
    return 1;
}

int video_config_gravar(FILE *f, const VideoPrefs *p) {
    if (!f || !p) return -1;
    if (fprintf(f, "video_escala=%d\nvideo_filtro=%d\nvideo_proporcao=%d\n",
                p->escala, p->filtro, p->proporcao) < 0) return -1;
    return fprintf(f, "video_shader=%d\n", p->shader) < 0 ? -1 : 0;
}
