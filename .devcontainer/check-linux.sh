#!/usr/bin/env bash
set -euo pipefail

# Codespaces Linux: validar sintaxe dos modulos portaveis.
# libretro_core.c usa LoadLibrary/GetProcAddress do Windows e e compilado
# no job Windows do GitHub Actions.
cd "$(dirname "$0")/.."
mapfile -t FLAGS < <(pkg-config --cflags sdl2 SDL2_image | tr ' ' '\n' | grep -v '^$')
files=(
  cartridge_style.c console_registry.c core_content.c core_paths.c save_manager.c image_processor.c input.c
  mini_frontend.c game_session.c ui_carousel.c ui_cards.c ui_game_view.c ui_menu.c
  ui_text.c ui_theme.c video_audio.c
)
for file in "${files[@]}"; do
  echo "Checando $file"
  gcc -std=gnu11 -fsyntax-only -Wall -Wextra "${FLAGS[@]}" "$file"
done
echo "Sintaxe dos modulos portaveis OK."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
gcc -std=gnu11 -Wall -Wextra -Werror -o "$test_dir/test_game_session" \
  game_session.c tests/test_game_session.c
"$test_dir/test_game_session"
gcc -std=gnu11 -Wall -Wextra -Werror -o "$test_dir/test_core_content" \
  core_content.c tests/test_core_content.c
"$test_dir/test_core_content"
gcc -std=gnu11 -Wall -Wextra -Werror -o "$test_dir/test_core_paths" \
  core_paths.c tests/test_core_paths.c
"$test_dir/test_core_paths"
gcc -std=gnu11 -Wall -Wextra -Werror -o "$test_dir/test_console_registry" \
  console_registry.c tests/test_console_registry.c
"$test_dir/test_console_registry"
gcc -std=gnu11 -Wall -Wextra -Werror -o "$test_dir/test_save_manager" \
  save_manager.c tests/test_save_manager.c
"$test_dir/test_save_manager"
echo "Compilacao Windows deve ser validada no Actions."
