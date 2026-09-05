#!/usr/bin/env bash
set -euo pipefail
root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$root_dir/scripts/qt-env.sh"
build_dir="${CHARGING_BUILD_DIR:-$root_dir/build/full}"
case "${1:-server}" in
  server)
    shift || true
    exec "$build_dir/apps/server/charging-server" --source-root "$root_dir" --web-root "$root_dir/web/dist" "$@"
    ;;
  user)
    shift
    exec "$build_dir/apps/user-app/charging-user" "$@"
    ;;
  admin)
    shift
    exec "$build_dir/apps/admin-app/charging-admin" "$@"
    ;;
  *)
    echo 'Usage: scripts/run.sh [server|user|admin] [arguments]' >&2
    exit 2
    ;;
esac
