#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential pkg-config libsdl2-dev libsdl2-image-dev \
  clang clang-format git

echo
echo "Ambiente Fofita preparado."
echo "Execute: bash .devcontainer/check-linux.sh"
echo "Para obter um EXE Windows: GitHub Actions > Build Fofita - Windows."
