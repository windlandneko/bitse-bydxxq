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

git -C "$repo_dir" ls-files --cached --others --exclude-standard -z -- \
  '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' \
  | while IFS= read -r -d '' path; do
      [[ "$path" == */third-party/* ]] && continue
      clang-format --style=file "${clang_args[@]}" "$repo_dir/$path"
    done
