#!/usr/bin/env bash
# Compila a librashader.dll a partir do codigo oficial, sem modificacoes, num
# commit fixado e com o Cargo.lock travado. Somente o runtime OpenGL e o CRT
# estatico: o binario oficial 0.12.0 exige D3DX9_43.dll (DirectX antigo, nao
# presente por padrao no Windows) e o Visual C++ Redistributable.
# Requer git, cargo/rustup e MSVC. Uso: compilar_librashader.sh <fonte> <destino>
set -euo pipefail
TAG=librashader-v0.12.0
COMMIT=87e8a97b50516d997defeaa168173dcd185d4022
TOOLCHAIN=${LIBRASHADER_RUST:?defina a versao do Rust}
ALVO=x86_64-pc-windows-msvc
RAIZ=$(cd "$(dirname "$0")/.." && pwd)
FONTE=${1:?pasta da fonte}
DESTINO=${2:?pasta de destino}

[ -d "$FONTE/.git" ] || git clone --depth 1 --branch "$TAG" \
  https://github.com/SnowflakePowered/librashader "$FONTE"
test "$(git -C "$FONTE" rev-parse HEAD)" = "$COMMIT" || {
  echo "::error::commit do librashader diferente do fixado"; exit 1; }
# O header versionado no Fofita deve ser o da mesma tag.
cmp <(tr -d '\r' < "$FONTE/include/librashader.h") \
    <(tr -d '\r' < "$RAIZ/third_party/librashader/librashader.h") || {
  echo "::error::header versionado difere do codigo fixado"; exit 1; }

rustup toolchain install "$TOOLCHAIN" --profile minimal --target "$ALVO"
(cd "$FONTE" && RUSTFLAGS="-C target-feature=+crt-static" \
  cargo "+$TOOLCHAIN" build --locked -p librashader-capi --profile optimized \
    --target "$ALVO" --no-default-features --features runtime-opengl)
test -z "$(git -C "$FONTE" status --porcelain --untracked-files=no)" || {
  echo "::error::a fonte do librashader foi modificada"; exit 1; }
mkdir -p "$DESTINO"
cp "$FONTE/target/$ALVO/optimized/librashader_capi.dll" "$DESTINO/librashader.dll"
sha256sum "$DESTINO/librashader.dll"
