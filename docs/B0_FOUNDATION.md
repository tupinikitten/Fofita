# Fofita — Entrega B0: sessão de jogo e capacidades dos cores

Implementação inicial da fase B0 na branch `feature/backend-foundation`.
A referência visual continua sendo o commit `666f0aa`, sem modificações em `ui_menu.c`, ícones ou layouts.

## Módulos

- `game_session.h/.c`: encapsula iniciar, executar frame, pausar, continuar e encerrar. Estados: MENU, STARTING, PLAYING, PAUSED, STOPPING, ERROR.
- `libretro_core.h/.c`: expõe `core_get_capabilities()`, um snapshot depois de carregar a ROM: disponibilidade de serialização de estado, tamanho em bytes, acesso à SRAM e presença da API de cheats.
- `mini_frontend.c`: usa a sessão para iniciar/parar/executar o jogo; mantém os caminhos de vídeo/áudio e o menu do build aprovado.
- `tests/test_game_session.c`: testes sem emulador real, cobrindo erros, retry, pausa, execução e limpeza.
- CI Windows e check Codespaces executam os testes unitários.

## Limitações intencionais

A B0 **não salva dados** e **não executa cheats**: detectar funções exportadas não prova que um código cheat é aceito. `retro_serialize_size()>0` significa que o core anunciou uma área de serialização após carregar o jogo, mas salvar/carregar depende da fase B2 e de validação real.

Nenhum sistema adicional foi ativado. O carregamento atual ainda lê ROM inteira na memória; o suporte `need_fullpath`, BIOS e discos pertence à fase B1. A pausa está disponível como estado do backend, mas não há menu rápido na interface.

## Validação

1. Verificar build `Build Fofita - Windows` no Actions.
2. Testar execução de NES com ROM legal que já funcionava.
3. Retornar ao menu com Esc e repetir três vezes.
4. Verificar áudio, vídeo, seleção de cartucho e controle.
5. Se houver regressão, reverter o commit B0 na branch de backend; não modificar `feature/ui-views-linux-preview` ou `main`.

## Próxima etapa

B1: adaptador portável para carregamento de cores, diretórios de sistema/salvamento, `need_fullpath`, metadados por sistema e testes de mais um core 2D. A futura camada de conquistas e HTTPS permanece isolada.
