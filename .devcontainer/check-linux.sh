#!/usr/bin/env bash
set -euo pipefail

# Codespaces Linux: validar sintaxe dos modulos portaveis.
# libretro_core.c usa LoadLibrary/GetProcAddress do Windows e e compilado
# no job Windows do GitHub Actions.
cd "$(dirname "$0")/.."
mapfile -t FLAGS < <(pkg-config --cflags sdl2 SDL2_image | tr ' ' '\n' | grep -v '^$')
files=(
  cartridge_style.c console_registry.c image_processor.c input.c
  mini_frontend.c ui_carousel.c ui_cards.c ui_game_view.c ui_menu.c
  ui_text.c ui_theme.c video_audio.c
)
for file in "${files[@]}"; do
  echo "Checando $file"
  gcc -std=gnu11 -fsyntax-only -Wall -Wextra "${FLAGS[@]}" "$file"
done
echo "Sintaxe dos modulos portaveis OK. Compilacao Windows deve ser validada no Actions."
