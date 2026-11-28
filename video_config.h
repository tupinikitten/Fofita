#ifndef VIDEO_CONFIG_H
#define VIDEO_CONFIG_H
#include <stdio.h>
/* Preferencias de video do jogo persistidas em fofita_menu.cfg. */
typedef struct {
    int escala;    /* 0 automatico, 1 inteira, 2 ajuste livre */
    int filtro;    /* 0 nitida, 1 suave */
    int proporcao; /* 0 original do console, 1 pixels quadrados */
    int shader;    /* ShaderModo (shader_manager.h) */
} VideoPrefs;

/* Aplica uma linha "chave=valor". Retorna 1 se a chave e de video. */
int video_config_ler_linha(VideoPrefs *prefs, const char *linha);
int video_config_gravar(FILE *arquivo, const VideoPrefs *prefs);
void video_config_padroes(VideoPrefs *prefs);
#endif
