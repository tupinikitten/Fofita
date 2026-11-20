# Fofita — correção de toques rápidos nos controles (B2)

## Sintoma relatado

Pokémon Red/Gold abrem com NEW GAME/OPTION, mas a seleção de OPTION pode
parecer ignorar os comandos. Zelda NES também aparenta não responder em
algumas confirmações. A confirmação do nome no Zelda NES tem uma mecânica
específica: SELECT até REGISTER END, seguido de START; isso não deve ser
confundido com falha no frontend.

## Causa técnica possível e correção defensiva

Antes desta alteração, a camada Libretro consultava exclusivamente
`SDL_GetKeyboardState` e `SDL_GameControllerGetButton`. A própria
documentação oficial SDL2 alerta que `SDL_GetKeyboardState` **perde toques
se a tecla for pressionada e solta antes de o estado ser consultado**:
https://wiki.libsdl.org/SDL2/SDL_GetKeyboardState

A função `input_latch_event` registra o evento SDL de pressionamento
antes de ser consumido pelo frontend. Em `retro_input_poll_cb`, o
`input_latch_begin_frame` disponibiliza esse aperto por um quadro
Libretro, mesmo quando a tecla foi solta rapidamente. A leitura de
estado atual continua sendo usada para comandos mantidos pressionados.

- Não altera botões padrão nem os perfis remapeados.
- Aplica-se a teclado e botões de gamepad; analógico continua no controle
  por eixo existente.
- Ignora KEYDOWN repetido do sistema para não gerar apertos extras.
- Cancela pulsos ao perder foco e ao iniciar/sair de um jogo.
- Não altera interface, saves ou comportamento do dashboard.
- A rotina antiga de Esc/Espaço e Back+Start continua igual.

## O que não foi demonstrado

O teste automatizado verifica o mecanismo de entrada (inclusive
KEYDOWN+KEYUP no mesmo ciclo de eventos), não reproduz as telas dos
jogos comerciais. O bug relatado só estará comprovadamente corrigido
quando a usuária conseguir usar OPTION nos Pokémon e o menu do Zelda
em um teste manual Windows.

## Roteiro breve

1. Restaurar controles padrão no submenu de Configurações.
2. Pokémon Red/Gold: na tela NEW GAME / OPTION, usar seta **baixo**
   e **X** (A) para confirmar. Testar pressões rápidas e prolongadas.
3. Zelda NES: escolher letras com setas + **X** (A); após registrar
   o nome, pressionar **Shift direito** (Select) até REGISTER END e
   **Enter** (Start) para confirmar. Esse é o procedimento previsto
   no manual original.
4. Para voltar ao dashboard a qualquer momento, usar **Esc/Espaço**;
   o item EXIT de um menu interno pertence ao jogo, não ao Fofita.
5. Depois de começar uma partida Pokémon, salvar no menu do próprio
   jogo (Start > SAVE). Só então CONTINUE aparece no título, ao reabrir.
