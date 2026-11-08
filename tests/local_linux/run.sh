#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
output="$(mktemp -d)"
trap 'rm -rf -- "$output"' EXIT
cc="${CC:-clang}"
python3 - "$root" "$output" <<'PYICON'
import pathlib, sys
sys.path.insert(0, str(pathlib.Path(sys.argv[1]) / "tools"))
import icon_png
(pathlib.Path(sys.argv[2]) / "icon_png.h").write_text(icon_png.c_array("ultrs_icon_png", icon_png.png(256)))
PYICON
common=(-I"$output" -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections)
graphics=(-I"$root/src/gfx" -I"$root/src/gfx/nvn" -I"$root/src/gfx/nvn/include" -I"$root/src/gfx/compat")
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/vpool.c" -o "$output/vpool" -lpthread
"$output/vpool"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/posix.c" -o "$output/posix" -lpthread
"$output/posix"
"$cc" "${common[@]}" "$root/tests/local_linux/guest_memory.c" -o "$output/guest_memory" -lpthread
"$output/guest_memory"
"$cc" "${common[@]}" "$root/tests/local_linux/romfs.c" -o "$output/romfs" -lpthread
"$output/romfs"
"$cc" "${common[@]}" "$root/tests/local_linux/fault_guard.c" -o "$output/fault_guard" -lpthread -ldl
"$output/fault_guard"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/buffer_updates.c" -o "$output/buffer_updates" -lpthread
"$output/buffer_updates"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/actor_callback.c" -o "$output/actor_callback" -lpthread
"$output/actor_callback"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/shader_layouts.c" -o "$output/shader_layouts" -lpthread
"$output/shader_layouts"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/present_retire.c" -o "$output/present_retire" -lpthread
"$output/present_retire"
"$cc" "${common[@]}" "${graphics[@]}" "$root/tests/local_linux/vertex_stride.c" -o "$output/vertex_stride" -lpthread
"$output/vertex_stride"
if sdl_config="$(pkg-config --cflags --libs sdl3 2>/dev/null)"; then
    read -r -a sdl_flags <<< "$sdl_config"
else
    sdl_flags=(-lSDL3)
fi
"$cc" "${common[@]}" "$root/tests/local_linux/controller.c" -o "$output/controller" "${sdl_flags[@]}" -lpthread -lm
"$output/controller"
printf '%s\n' 'Linux memory-pool, guest-page reuse, POSIX signal/resource, missing-file, fault-guard, and controller regression checks passed.'

if command -v gdb >/dev/null; then
    python3 "$root/tests/local_linux/debug_runner.py"
else
    printf '%s\n' 'GDB unavailable: automatic debugger tests skipped.'
fi
