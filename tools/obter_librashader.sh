#!/usr/bin/env bash
# Baixa o binario oficial do librashader numa versao fixa, confere o SHA-256 e
# extrai somente a DLL. Uso: tools/obter_librashader.sh <pasta_destino>
set -euo pipefail
VERSAO=0.12.0
ZIP="librashader-x86_64-windows-v${VERSAO}-optimized.zip"
URL="https://github.com/SnowflakePowered/librashader/releases/download/librashader-v${VERSAO}/${ZIP}"
SHA256=521fe0f364bfa705883f9e99fb1733a3a24f0f403bcc5591b6b78f6cff289183
RAIZ=$(cd "$(dirname "$0")/.." && pwd)
DESTINO=${1:?informe a pasta de destino}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

curl --fail --location --proto '=https' --tlsv1.2 --retry 3 -sS -o "$TMP/$ZIP" "$URL"
echo "$SHA256  $TMP/$ZIP" | sha256sum -c -
unzip -q "$TMP/$ZIP" librashader.dll librashader.h -d "$TMP/x"

# O header versionado no repositorio deve ser a mesma API do binario.
macros() { tr -d '\r' < "$1" | grep -E '^#define LIBRASHADER_CURRENT_(VERSION|ABI) '; }
# Sem diff/cmp: o MSYS2 do CI nao inclui diffutils.
esperado=$(macros "$RAIZ/third_party/librashader/librashader.h")
obtido=$(macros "$TMP/x/librashader.h")
if [ -z "$obtido" ] || [ "$obtido" != "$esperado" ]; then
  echo "::error::ABI/API do header versionado difere do binario baixado"
  printf 'versionado:\n%s\nbinario:\n%s\n' "$esperado" "$obtido"
  exit 1
fi
resumo() { tr -d '\r' < "$1" | sha256sum; }
if [ "$(resumo "$TMP/x/librashader.h")" = "$(resumo "$RAIZ/third_party/librashader/librashader.h")" ]; then
  echo "Header do zip identico ao versionado"
else
  echo "Aviso: header do zip difere em texto do versionado (ABI/API iguais)"
fi
mkdir -p "$DESTINO"
cp "$TMP/x/librashader.dll" "$DESTINO/librashader.dll"
echo "librashader $VERSAO obtido em $DESTINO"
