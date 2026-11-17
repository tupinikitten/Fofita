# Fofita — consolidação dos arquivos de UI

**Base:** branch `feature/ui-views-linux-preview`, derivada de `setup/ci-windows-codespaces`.

Este ZIP contém só os arquivos modificados pelas últimas revisões de UI:
- `ui_menu.c`, `ui_menu.h` — menu lateral V3, presets e interações
- `ui_text.c`, `ui_text.h`, `ui/font/lato_atlas.png` — fonte atlas
- `video_audio.c`, `video_audio.h` — canvas lógico para dashboard / saída física para jogos
- `mini_frontend.c` — eventos e renderização
- `input.c` — controles SDL2

O conteúdo foi consolidado a partir dos pacotes de revisão já enviados nesta conversa. **Ainda não foi compilado nem testado junto à branch atual.**

## Aplicação no Codespaces (branch correta)
1. Abra o Codespace ou mude para a branch `feature/ui-views-linux-preview`.
2. Arraste o arquivo ZIP para o explorador de arquivos do Codespaces, na raiz do repositório.
3. No terminal, execute:

   ```sh
   unzip -o Fofita_consolidacao_UI_para_Codespaces.zip
   bash .devcontainer/check-linux.sh
   git status --short
   ```

4. Se o check passar, envie as mudanças:

   ```sh
   git add ui_menu.c ui_menu.h ui_text.c ui_text.h ui/font/lato_atlas.png video_audio.c video_audio.h mini_frontend.c input.c
   git commit -m "feat: consolidar redesign e canvas logico"
   git push origin feature/ui-views-linux-preview
   ```

5. Abra **Actions** para acompanhar o build de Windows. Se falhar, use o log da execução para orientar a correção.

**Observação:** o Codespaces valida sintaxe dos módulos portáveis; não substitui teste do executável Windows ou da UI no Switch. Não envie ROMs ao GitHub.
