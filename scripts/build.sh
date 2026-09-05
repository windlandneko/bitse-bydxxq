#!/usr/bin/env bash
set -euo pipefail
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source-path=SCRIPTDIR
# shellcheck source=qt-env.sh
source "$root_dir/scripts/qt-env.sh"
cd "$root_dir"
for command_name in cmake pnpm uv; do
  command -v "$command_name" >/dev/null || { echo "Missing $command_name; run scripts/setup_env.sh." >&2; exit 1; }
done
build_dir="${CHARGING_BUILD_DIR:-$root_dir/build/full}"
[[ "$build_dir" == /* ]] || build_dir="$root_dir/$build_dir"

# Install Python dependencies before CMake registers their tests.
pnpm --dir "$root_dir/web" install --frozen-lockfile
uv sync --project "$root_dir/ml" --frozen
args=(-S "$root_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="${CHARGING_BUILD_TYPE:-RelWithDebInfo}" -DBUILD_TESTING=ON -DBUILD_USER_APP=ON -DBUILD_ADMIN_APP=ON -DBUILD_SERVER=ON -DBUILD_MOBILE_FLOW_TESTS=ON)
if [[ -n "${CHARGING_QT_PREFIX:-}" ]]; then
  args+=(-DQT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES=ON)
  for module in "$CHARGING_QT_LIB_DIR"/cmake/Qt6*; do
    [[ -d "$module" ]] && args+=("-D$(basename "$module")_DIR=$module")
  done
fi
cmake "${args[@]}" "$@"
cmake --build "$build_dir" --parallel "${CHARGING_BUILD_JOBS:-4}"
pnpm --dir "$root_dir/web" build

# The optional prefix contains Ubuntu Qt add-ons, while base Qt/platform plugins
# remain installed by apt. Qt 6.2 WebEngine needs qt.conf in BOTH processes.
if [[ -n "${CHARGING_QT_PREFIX:-}" && -x "$CHARGING_QT_PREFIX/lib/qt6/libexec/QtWebEngineProcess" ]]; then
  plugin_dir="$(qmake6 -query QT_INSTALL_PLUGINS)"
  for config_dir in "$build_dir/apps/user-app" "$build_dir/apps/admin-app" "$build_dir/apps/server" "$CHARGING_QT_PREFIX/lib/qt6/libexec"; do
    [[ -d "$config_dir" ]] || continue
    cat > "$config_dir/qt.conf" <<CONFIG
[Paths]
Prefix=$CHARGING_QT_PREFIX
Data=share/qt6
Translations=share/qt6/translations
LibraryExecutables=lib/qt6/libexec
Plugins=$plugin_dir
Qml2Imports=$CHARGING_QT_QML_DIR
CONFIG
  done
fi
printf 'Build ready: %s\n' "$build_dir"
