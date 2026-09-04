#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

if ! command -v clang-format >/dev/null 2>&1; then
  echo 'clang-format was not found. Install it with: sudo apt install clang-format' >&2
  exit 127
fi

case ${1:-format} in
  format) clang_args=(-i) ;;
  --check) clang_args=(--dry-run --Werror) ;;
  *) echo "Usage: $0 [--check]" >&2; exit 2 ;;
esac

find "$repo_dir" \
  -type d \( -name .git -o -name build -o -name 'build-*' -o -name 'cmake-build-*' \) -prune \
  -o -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \
  -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) \
  -exec clang-format --style=file "${clang_args[@]}" {} +
