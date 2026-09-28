#!/usr/bin/env bash
set -euo pipefail

# Install only into the exact Steam build used to make this mod.
if (($# != 1)); then
  printf 'Uso: %s /caminho/para/KingdomComeDeliverance2\n' "$0" >&2
  exit 2
fi

game_dir=$1
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
binary_dir=$game_dir/Bin/Win64MasterMasterSteamPGO
game_dll=$binary_dir/WHGame.dll
mod_file=$project_dir/dist/InstantPrepare.asi
loader_file=$project_dir/dist/dinput8.dll
expected_game_hash=bdf8f9e4a11257a72b64c84700e284c29e4c4ccaf5b8d4bfa7d0b2a7294479f7
expected_loader_hash=fa266e3513d02c08a1b808f28c10538a489eaffaa4b0707f7cc1066e71b5afd7

for required in "$game_dll" "$mod_file" "$loader_file"; do
  if [[ ! -f $required ]]; then
    printf 'Arquivo ausente: %s\n' "$required" >&2
    exit 1
  fi
done

if [[ $(sha256sum "$game_dll" | cut -d ' ' -f 1) != $expected_game_hash ]]; then
  printf 'WHGame.dll diferente da montagem 15693 validada; instalação cancelada.\n' >&2
  exit 1
fi
if [[ $(sha256sum "$loader_file" | cut -d ' ' -f 1) != $expected_loader_hash ]]; then
  printf 'Carregador ASI diferente do arquivo validado; instalação cancelada.\n' >&2
  exit 1
fi

if [[ -e $binary_dir/dinput8.dll || -e $binary_dir/InstantPrepare.asi ]]; then
  printf 'Já existe um carregador ou mod com esse nome; instalação cancelada.\n' >&2
  exit 1
fi

if ps -eo comm= | rg -iq '^KingdomCome(\.exe)?$'; then
  printf 'Feche o jogo antes de instalar o mod.\n' >&2
  exit 1
fi

install -m 0644 "$loader_file" "$binary_dir/dinput8.dll"
if ! install -m 0644 "$mod_file" "$binary_dir/InstantPrepare.asi"; then
  rm -- "$binary_dir/dinput8.dll"
  printf 'Falha ao copiar a extensão; o carregador copiado foi removido.\n' >&2
  exit 1
fi

printf 'Instalado em %s\n' "$binary_dir"
sha256sum "$binary_dir/dinput8.dll" "$binary_dir/InstantPrepare.asi"
