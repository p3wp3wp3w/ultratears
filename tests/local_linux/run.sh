#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
output="$(mktemp -d)"
trap 'rm -rf -- "$output"' EXIT
cc="${CC:-clang}"
common=(-O2 -ffunction-sections -fdata-sections -Wl,--gc-sections)
graphics=(-I"$root/src/gfx" -I"$root/src/gfx/nvn" -I"$root/src/gfx/nvn/include" -I"$root/src/gfx/compat")
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/vpool.c" -o "$output/vpool" -lpthread
"$output/vpool"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/posix.c" -o "$output/posix" -lpthread
"$output/posix"
"$cc" "${common[@]}" "$root/tests/local_linux/romfs.c" -o "$output/romfs" -lpthread
"$output/romfs"
"$cc" "${common[@]}" "$root/tests/local_linux/fault_guard.c" -o "$output/fault_guard" -lpthread -ldl
"$output/fault_guard"
if sdl_config="$(pkg-config --cflags --libs sdl3 2>/dev/null)"; then
    read -r -a sdl_flags <<< "$sdl_config"
else
    sdl_flags=(-lSDL3)
fi
"$cc" "${common[@]}" "$root/tests/local_linux/controller.c" -o "$output/controller" "${sdl_flags[@]}" -lpthread
"$output/controller"
printf '%s\n' 'Linux memory-pool, POSIX signal/resource, missing-file, fault-guard, and controller regression checks passed.'

if command -v gdb >/dev/null; then
    python3 "$root/tests/local_linux/debug_runner.py"
else
    printf '%s\n' 'GDB unavailable: automatic debugger tests skipped.'
fi
