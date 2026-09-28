#!/usr/bin/env bash
set -euo pipefail

# Cross-compile the Windows ASI plugin from Linux. The compiler stays external.
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
compiler=${CC:-x86_64-w64-mingw32-gcc-posix}
compiler_path=$(command -v "$compiler")
bin_dir=$(dirname -- "$compiler_path")

"$compiler_path" \
  -B"$bin_dir/" \
  -std=c11 -O2 -Wall -Wextra -Werror -static-libgcc -shared \
  -I"$project_dir/third_party/minhook/include" \
  -I"$project_dir/third_party/minhook/src" \
  -I"$project_dir/src" \
  -o "$project_dir/dist/InstantPrepare.asi" \
  "$project_dir/src/instant_prepare.c" \
  "$project_dir/third_party/minhook/src/buffer.c" \
  "$project_dir/third_party/minhook/src/hook.c" \
  "$project_dir/third_party/minhook/src/trampoline.c" \
  "$project_dir/third_party/minhook/src/hde/hde64.c"

file "$project_dir/dist/InstantPrepare.asi"
