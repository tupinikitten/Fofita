# B1.2 — Preparação do Game Boy e Game Boy Color

**Branch:** `feature/backend-foundation`  
**Base aprovada:** B1.1, commit `e0f0ae6`.

## Decisão

Usar **Gambatte** como primeiro core Libretro para Game Boy e Game Boy Color. São **dois sistemas e uma DLL**, mas cada um preserva seu ID, pasta de ROMs, ícone e biblioteca de jogos:

- Game Boy: `roms/gb` — extensões `.gb` e `.dmg`.
- Game Boy Color: `roms/gbc` — extensão `.gbc`.
- Core compartilhado: `cores/gambatte_libretro.dll`.

A documentação oficial confirma suporte a Game Boy/Color, BIOS opcionais, saves e estados (a implementação de saves/estados no Fofita ainda está pendente).

## O que este commit entrega

- Caminhos Gambatte configurados para os dois consoles no `console_registry.c`.
- Testes automáticos do registro, extensões e proteção da lista de 14 sistemas.
- Instruções em `cores/README.md` para instalar uma DLL Libretro Windows legítima e realizar os testes no PC.

## O que permanece pendente

- A DLL do Gambatte **não foi adicionada**; nenhum core novo estará disponível até uma versão legítima/compatível ser instalada. A CI compila o Fofita e valida o registro, **não valida gameplay de GB/GBC**.
- A API Libretro oferece overrides de conteúdo por extensão (`RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE`). O frontend ainda não declara suporte: não retornar sucesso sem implementar também a consulta de informações estendidas, exigida por cores que utilizem essa API.
- Suporte completo às opções dos cores (paletas de Game Boy, colorização, BIOS e opções de vídeo) será desenvolvido separadamente.
- SRAM persistida e savestates continuam na etapa B2; a presença da API de um core não significa salvamento implementado no Fofita.

## Checklist manual após obter a DLL

1. Abrir uma ROM NES: confirmar que o jogo e a volta ao menu continuam normais.
2. Copiar `gambatte_libretro.dll` para `cores/` e colocar ROMs legais `.gb` e `.gbc` nas pastas correspondentes.
3. Reiniciar Fofita e confirmar que **Game Boy** e **Game Boy Color** aparecem individualmente, com suas miniaturas.
4. Iniciar jogo GB, controlar, sair. Repetir para GBC.
5. Verificar ausência de travamentos ou perda de configurações visuais.
6. **Não** considerar essa etapa validada apenas pelo sucesso de GitHub Actions: os testes de execução dos novos cores dependem da DLL e de ROMs de teste locais.

Referência: https://docs.libretro.com/library/gambatte/
