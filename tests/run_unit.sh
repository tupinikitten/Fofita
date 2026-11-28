#!/usr/bin/env bash
# Compila e executa os testes sem GPU (Linux ou MSYS2).
# Variaveis opcionais: CC, SDL_CFLAGS, SDL_LIBS, GL_LIBS, EXTRA_CFLAGS.
set -euo pipefail
cd "$(dirname "$0")/.."
CC=${CC:-gcc}
SDL_CFLAGS=${SDL_CFLAGS-$(pkg-config --cflags sdl2)}
SDL_LIBS=${SDL_LIBS-$(pkg-config --libs sdl2)}
case "$(uname -s)" in
  MINGW*|MSYS*) EXT=dll; GL_LIBS=${GL_LIBS--lopengl32} ;;
  *) EXT=so; GL_LIBS=${GL_LIBS--lGL} ;;
esac
OUT=build/unit
mkdir -p "$OUT"
FLAGS="-std=gnu11 -g -Wall -Wextra -Werror ${EXTRA_CFLAGS:-}"
$CC $FLAGS -shared -fPIC -DSTUB_ABI=99 tests/stub_librashader.c -o "$OUT/stub_abi.$EXT"
$CC $FLAGS -shared -fPIC tests/stub_librashader.c -o "$OUT/stub_incompleto.$EXT"
$CC $FLAGS $SDL_CFLAGS tests/test_unit.c video_math.c video_config.c \
    shader_manager.c slang_backend.c -o "$OUT/test_unit" $SDL_LIBS $GL_LIBS -lm
cd "$OUT"
./test_unit . "$PWD/stub_abi.$EXT" "$PWD/stub_incompleto.$EXT"
