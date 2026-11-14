#!/usr/bin/env bash
# Compila o .3dsx e empacota o .cia usando Docker (nao precisa instalar devkitPro).
set -euo pipefail
cd "$(dirname "$0")"

MAKEROM_URL="https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.19.0/makerom-v0.19.0-ubuntu_x86_64.zip"
PLATFORM=""
[ "$(uname -m)" = "arm64" ] && PLATFORM="--platform linux/amd64"

# roda o uid do host para os arquivos nao sairem como root
USERFLAG=(--user "$(id -u):$(id -g)")

echo "== compilando (devkitARM no Docker)"
docker run --rm $PLATFORM "${USERFLAG[@]}" -v "$PWD":/work -w /work devkitpro/devkitarm \
  bash -c 'export PATH=$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH; make -j"$(nproc)"'

echo "== empacotando .cia"
if [ ! -x .tools/makerom ]; then
  mkdir -p .tools
  curl -fsSL -o .tools/makerom.zip "$MAKEROM_URL"
  unzip -oq .tools/makerom.zip -d .tools
fi

# makerom 0.19 precisa de glibc >= 2.38, entao roda num ubuntu 24.04
docker run --rm $PLATFORM "${USERFLAG[@]}" -v "$PWD":/work -w /work ubuntu:24.04 \
  bash -c 'chmod +x .tools/makerom && .tools/makerom -f cia -o immich-3ds.cia \
    -elf immich-3ds.elf -icon immich-3ds.smdh -banner meta/banner.bnr \
    -desc app:2 -rsf app.rsf -target t'

echo
echo "ok:"
ls -lh immich-3ds.cia immich-3ds.3dsx
