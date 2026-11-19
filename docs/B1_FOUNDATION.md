# Fofita — Entrega B1.1: caminhos Libretro e carregamento de conteúdo

**Branch de implementação:** `feature/backend-foundation`.
**Base já validada pela usuária:** B0, commit `cf2efbe`.
**Arquivos visuais:** nenhum arquivo da UI foi alterado.

## O que foi implementado

- `core_content.c/h` separa ROMs **em memória** (conteúdo inteiro lido) de cores com `retro_system_info.need_fullpath = true` (somente caminho; `data=NULL` e `size=0`). Mantém o caminho válido até descarregar o jogo e libera memória no fechamento.
- `libretro_core.c` carrega `retro_get_system_info` e usa `need_fullpath` **antes** de `retro_load_game`. Os demais callbacks de vídeo, áudio e input continuam como na B0.
- `core_paths.c/h` monta diretórios persistentes, acessíveis via `RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY` e `RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY`. A pasta base vem de `SDL_GetPrefPath("TupiniKitten","Fofita")`, não do diretório do executável.
- `close_core()` executa `retro_deinit()` somente se `retro_init()` foi realmente chamado, inclusive nos caminhos de erro.
- Testes automáticos validam arquivo por memória/fullpath, diretórios e B0 no Windows e em verificação de sintaxe do Codespaces.

## Onde colocar BIOS e saves

O SDL2 escolhe uma pasta **por usuário** (por exemplo, no Windows, uma pasta em `%APPDATA%\TupiniKitten\Fofita`). O caminho exato depende do sistema e das configurações do SDL.

- `system/`: BIOS e arquivos de sistema que **o usuário possui legalmente**; não serão redistribuídos pelo Fofita.
- `saves/`: dados criados por cores que usam a API de diretório de saves.

Não há ainda interface gráfica para localizar essas pastas. O menu atual e as pastas `roms/<id>` não mudaram.

**Importante:** informar o diretório ao core **não grava SRAM automaticamente**. Salvamentos normais por `retro_get_memory_data()` e os savestates serão implementados na etapa B2.

## Limitações da B1.1

- Apenas o NES continua com core configurado, sem adicionar DLLs ou ativar outros consoles.
- Cores que fornecem `RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE` por extensão ainda precisam de tratamento específico; por enquanto é respeitado o valor geral de `need_fullpath`.
- Para ROMs lidas inteiras em RAM, o código ainda depende de `fseek`/`ftell`. Arquivos muito grandes e compactações requerem suporte posterior; não forçar o carregamento total se o core pede caminho.
- Não foram implementados gerenciamento de BIOS na UI, SRAM persistida, memória de cartões, shaders ou rede.
- O mecanismo `LoadLibraryA` ainda é específico de Windows; portabilidade para Switch precisa de adaptação de plataforma e testes reais.

## Checklist de validação

1. O GitHub Actions precisa compilar Fofita e concluir os testes B0 + B1.
2. Abrir uma ROM NES já funcional e verificar áudio, vídeo e controles.
3. Voltar ao menu e repetir algumas vezes; não perder a seleção nem as preferências.
4. Verificar que o diretório persistente foi criado e que `system/` e `saves/` estão acessíveis.
5. Confirmar que um core que pede `need_fullpath` não provoca leitura de um disco inteiro para a RAM, usando um core de teste antes de ativar consoles adicionais.

## B1.2 (próxima etapa)

Tratar opções avançadas e overrides de conteúdo por extensão, melhorar mapeamento de erros e selecionar os cores de GB/GBC de acordo com compatibilidade, distribuição e testes. Integração de RetroAchievements permanece posterior, com rede/TLS e credenciais em módulos separados.
