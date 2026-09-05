#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# This list is also consumed by CI; install QML runtime modules explicitly.
apt_packages=(
  build-essential cmake ninja-build pkg-config clang-format clangd gdb
  git curl ca-certificates xz-utils python3 python3-pip python3-venv
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-declarative-dev-tools
  qt6-webengine-dev qt6-webengine-dev-tools qt6-positioning-dev
  libqt6charts6-dev libqt6svg6-dev libqt6sql6-sqlite libqt6opengl6-dev
  libqt6webenginecore6-bin
  qml6-module-qtqml qml6-module-qtqml-models qml6-module-qtqml-workerscript
  qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts
  qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtquick-nativestyle
  fonts-noto-cjk fonts-dejavu-core libgl1-mesa-dri libgl-dev libegl-dev libopengl-dev
  xvfb xauth shellcheck
)
mode="${1:---all}"
case "$mode" in
  --print-packages) printf '%s\n' "${apt_packages[@]}"; exit 0 ;;
  --help|-h)
    echo 'Usage: scripts/setup_env.sh [--all|--system-only|--tools-only|--check|--print-packages]'
    echo '--all: apt system dependencies plus pinned user-local Node, pnpm and uv.'
    echo '--check: read-only tool validation, including an optional local Qt prefix.'
    exit 0
    ;;
  --all|--system-only|--tools-only|--check) ;;
  *) echo "Unknown option: $mode" >&2; exit 2 ;;
esac
if (( $# > 1 )); then echo 'Expected at most one option.' >&2; exit 2; fi

if [[ "$mode" == --all || "$mode" == --system-only ]]; then
  if ! command -v apt-get >/dev/null; then
    echo 'This installer targets Ubuntu 22.04 or newer with apt-get.' >&2
    exit 1
  fi
  privilege=()
  if (( EUID != 0 )); then
    if ! command -v sudo >/dev/null; then
      echo 'System package installation requires sudo; --check and --tools-only need no root.' >&2
      exit 1
    fi
    privilege=(sudo)
  fi
  "${privilege[@]}" apt-get update
  "${privilege[@]}" apt-get install -y --no-install-recommends "${apt_packages[@]}"
  [[ "$mode" == --system-only ]] && exit 0
fi

# shellcheck source-path=SCRIPTDIR
# shellcheck source=qt-env.sh
source "$root_dir/scripts/qt-env.sh"
if [[ "$mode" != --check ]]; then
  if (( EUID == 0 )); then
    echo 'Install user tools as your normal account: scripts/setup_env.sh --tools-only' >&2
    exit 1
  fi
  for command_name in curl python3 tar sha256sum; do
    command -v "$command_name" >/dev/null || { echo "Missing $command_name; install system dependencies first." >&2; exit 1; }
  done
  tools_dir="$HOME/.local/opt/charging-tools"
  node_version=24.20.0
  uv_version=0.12.9
  pnpm_version=10.33.0
  mkdir -p "$tools_dir"
  if ! command -v node >/dev/null || [[ "$(node --version)" != "v$node_version" ]]; then
    case "$(uname -m)" in
      x86_64) node_arch=x64 ;;
      aarch64|arm64) node_arch=arm64 ;;
      *) echo 'Install Node.js 24 manually for this architecture.' >&2; exit 1 ;;
    esac
    node_dir="$tools_dir/node-v$node_version"
    if [[ ! -x "$node_dir/bin/node" ]]; then
      download_dir="$(mktemp -d)"
      trap 'rm -rf "$download_dir"' EXIT
      archive="node-v$node_version-linux-$node_arch.tar.xz"
      curl --fail --silent --show-error --location "https://nodejs.org/dist/v$node_version/$archive" --output "$download_dir/$archive"
      curl --fail --silent --show-error --location "https://nodejs.org/dist/v$node_version/SHASUMS256.txt" --output "$download_dir/SHASUMS256.txt"
      awk -v archive="$archive" '$2 == archive { print; found=1 } END { if (!found) exit 1 }' "$download_dir/SHASUMS256.txt" > "$download_dir/checksum.txt"
      (cd "$download_dir" && sha256sum --check checksum.txt)
      mkdir -p "$download_dir/extracted"
      tar -xJf "$download_dir/$archive" -C "$download_dir/extracted" --strip-components=1
      mv "$download_dir/extracted" "$node_dir"
    fi
    export PATH="$node_dir/bin:$PATH"
  fi
  if ! command -v pnpm >/dev/null || [[ "$(pnpm --version)" != "$pnpm_version" ]]; then
    npm install --global --prefix "$tools_dir/pnpm" "pnpm@$pnpm_version"
    export PATH="$tools_dir/pnpm/bin:$PATH"
  fi
  if ! command -v uv >/dev/null || [[ "$(uv --version)" != "uv $uv_version"* ]]; then
    python3 -m venv "$tools_dir/uv"
    "$tools_dir/uv/bin/python" -m pip install --disable-pip-version-check "uv==$uv_version"
    export PATH="$tools_dir/uv/bin:$PATH"
  fi
fi

for command_name in cmake c++ clang-format python3 node pnpm uv; do
  if ! command -v "$command_name" >/dev/null; then
    echo "Missing $command_name. Run scripts/setup_env.sh or install it manually." >&2
    exit 1
  fi
done
node -e "if (Number(process.versions.node.split('.')[0]) < 22) { console.error('Node.js 22 or newer is required'); process.exit(1) }"
python3 -c "import sys; assert sys.version_info >= (3, 10), 'Python 3.10 or newer is required'"
node --version
pnpm --version
uv --version
if [[ -n "${CHARGING_QT_PREFIX:-}" ]]; then
  echo "Using optional Qt add-ons: $CHARGING_QT_PREFIX"
fi
echo 'Tools are ready. scripts/build.sh performs the complete Qt module and runtime build checks.'
