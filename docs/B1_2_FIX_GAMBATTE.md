# Correção B1.2 — Gambatte recusando ROMs

## Diagnóstico

No pacote fornecido pela usuária, `gambatte_libretro.dll` está presente ao lado do core NES, e os jogos de GB/GBC examinados possuem cabeçalhos identificáveis. A mensagem "Falha ao abrir a ROM" ocorre depois de `init_core()` e indica falha em `load_game()`.

A versão do Gambatte distribuída contém o texto **"Cannot dupe frames!"** e, no código-fonte do core, `retro_load_game()` rejeita o jogo se a chamada `RETRO_ENVIRONMENT_GET_CAN_DUPE` não retornar sucesso e um booleano true.

O callback do Fofita só atendia geometria, formato de pixel, diretórios de sistema e saves. Agora responde a `RETRO_ENVIRONMENT_GET_CAN_DUPE`, já que `retro_video_refresh_cb()` mantém a textura existente quando `data==NULL`. Esta é a implementação requerida por essa consulta.

## Testes

O GitHub Actions compila Fofita e executa `tests/test_core_environment.c`, verificando que a callback retorna true, escreve true na variável e rejeita ponteiros nulos. O teste também preserva a consulta do formato de pixel RGB565.

**Ainda é necessário validar gameplay no Windows com a DLL Gambatte real.** Este commit não distribui ROMs nem altera a UI.

## Conteúdo organizado

Os arquivos `.gb` pertencem a `roms/gb/` e os `.gbc` a `roms/gbc/`. Arquivos `.gb` que estiverem em `roms/gbc/` são ignorados pelo filtro atual.

## Como verificar

1. Baixar o novo pacote Windows desta branch.
2. Extrair em pasta nova e copiar `gambatte_libretro.dll` para `cores/` (além das ROMs legais nas pastas de GB/GBC).
3. Iniciar um jogo GB e um GBC; testar áudio, vídeo, teclado/controle e voltar ao menu.
4. Confirmar que o NES continua funcionando.
