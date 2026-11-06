#ifndef UI_MENU_H
#define UI_MENU_H

#include <SDL2/SDL.h>
#include "console_registry.h"

typedef enum {
    MENU_ACAO_NENHUMA = 0,
    MENU_ACAO_INICIAR_JOGO,   /* iniciar o jogo em foco no console selecionado */
    MENU_ACAO_SAIR            /* fechar o Fofita */
} MenuAcao;

/* Precisa ser chamado depois de init_video_audio() e init_carousel(). */
int  ui_menu_init(void);
void ui_menu_fechar(void);

/* Navegacao por teclado. Devolve o que o main precisa fazer em seguida. */
MenuAcao ui_menu_tecla(SDL_Keycode tecla, Uint16 modificadores);

void ui_menu_desenhar(void);

/* Console selecionado no cabecalho (NULL se nenhum estiver visivel). */
const ConsoleConfig *ui_menu_console_atual(void);

/* Volta o foco para o carrossel (usado ao retornar de um jogo). */
void ui_menu_focar_carrossel(void);

/* Mensagem temporaria no topo da area do carrossel. */
void ui_menu_aviso(const char *mensagem);

#endif
