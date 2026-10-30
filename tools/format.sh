#!/usr/bin/env bash
#
# format.sh — run clang-format-18 on every git-tracked *.cpp and *.hpp file.
#
# Usage:
#   tools/format.sh          format the files in place
#   tools/format.sh --check  modify nothing; exit non-zero if any file is not
#                            already formatted
#
set -euo pipefail

CLANG_FORMAT="clang-format-18"

if ! command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
    echo "error: '$CLANG_FORMAT' not found in PATH" >&2
    exit 1
fi

case "${1:-}" in
    "")       args=(-i) ;;
    --check)  args=(--dry-run --Werror) ;;
    *)        echo "usage: $0 [--check]" >&2; exit 2 ;;
esac

repo_root=$(git rev-parse --show-toplevel)

cd "$repo_root"
git ls-files -z -- '*.cpp' '*.hpp' | xargs -0 -r "$CLANG_FORMAT" --style=file:.clang-format "${args[@]}"
