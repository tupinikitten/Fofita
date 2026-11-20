# Correção de controles — Pokémon e Zelda (B2.1)

**Objetivo:** menus de jogos (nome de personagem, opções, caixas de diálogo)
devem receber os botões Libretro corretamente, sem serem tratados como comandos
do dashboard. Sem alterações visuais na interface aprovada.

## Problemas identificados

1. `mini_frontend.c` convertia `SDL_CONTROLLER_BUTTON_B` em `SDLK_ESCAPE`
   mesmo com o jogo aberto. Isso executava `voltar_ao_menu()` no primeiro
   toque de B, em vez de permitir que o core recebesse o botão de jogo.
2. A navegação no dashboard usava o analógico esquerdo, mas `input.c`
   encaminhava ao Libretro somente o D-pad digital. Logo, menus de jogos
   não podiam ser navegados pelo stick.

## Solução

- No jogo, os eventos de controle não são mais traduzidos em atalhos de menu.
  O core continua lendo A, B, Start, Select e D-pad pelo callback Libretro.
- O stick esquerdo passa a gerar direções Libretro, com zona morta de 16000.
- Sair com controle: **Back/Select + Start juntos** (não apenas B).
- Sair com teclado: **Esc** ou **Espaço**, comportamento anterior preservado.
- Teclado dentro do jogo: **setas** = direções; **X** = A; **Z** = B;
  **Enter** = Start; **Shift direito** = Select.
- A navegação normal da sidebar e dos consoles não foi alterada.
- Testes unitários cobrem a decisão de saída e a zona morta do analógico.

## Teste manual sugerido

1. Abrir Pokémon Gold com controle e acessar o item Options usando stick ou D-pad e A.
2. Em Zelda, testar digitar nome, usar A para confirmar, B para corrigir,
   Start para terminar conforme o jogo.
3. Garantir que B **não volta ao Fofita** enquanto o jogo está aberto.
4. Testar Back+Start para retornar e verificar que NES, GB e GBC continuam funcionando.
5. Repetir com teclado se possível, para distinguir problema de controle de problema da ROM.

O GitHub Actions verifica o mapeamento estático e a compilação, não simula
uma partida inteira. O resultado ainda depende da validação Windows.
