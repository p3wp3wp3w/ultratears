#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
profile="${TOTK_PROFILE_DIR:-$root/userdata}"
mkdir -p "$profile/logs" "$profile/cache"
if [[ "${1:-}" == --logs ]]; then exec xdg-open "$profile/logs"; fi
export TOTK_PROFILE_DIR="$profile"
export LARECOMP_DATA_ROOT="$profile/logs"
export TOTK_SHADER_BIN="$profile/cache/compiled.bin"
export TOTK_FPS_LIMIT="${TOTK_FPS_LIMIT:-60}"
export TOTK_FULLSCREEN="${TOTK_FULLSCREEN:-1}"
export TOTK_OPERATION_MODE="${TOTK_OPERATION_MODE:-handheld}"
export TOTK_CONTROLLER_LAYOUT="${TOTK_CONTROLLER_LAYOUT:-xbox-letters}"
export TOTK_RUMBLE="${TOTK_RUMBLE:-1}"
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER:-x11}"
exec 9>"$profile/.ultratears.lock"
if ! flock -n 9; then
    command -v notify-send >/dev/null && notify-send UltraTears 'UltraTears is already running.'
    exit 0
fi
cd "$root"
if [[ "${1:-}" == --debug ]]; then
    shift
    exec python3 "$root/tools/debug_run.py" --game "$root/ultratears" --log-root "$profile/logs" -- "$@"
fi
exec "$root/ultratears" "$@"
