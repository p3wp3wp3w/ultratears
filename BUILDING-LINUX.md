# Experimental Linux build and diagnostics

This branch builds a native x86-64 Linux executable from a validated Tears of the Kingdom 1.4.3 dump. It remains experimental; successful startup or one save-load test is not a complete playthrough check.

## Toolchain and build

Use Clang 22 or newer, LLD, Ninja, Python 3.14 and the SDL3, Opus, libvpx, Zstandard and Vulkan development libraries. Building the shader translator also needs the .NET 10 SDK. The local verified compiler is Clang 22.1.2; the earlier mixed Clang 18/22 build had compiler crashes and runtime failures. Rebuild all generated objects when changing compiler versions, since Ninja's literal `clang` command does not track the compiler binary behind that name.

Follow the existing dump layout in README.md, then run:

```sh
python3 tools/setup.py --dump /path/to/your/extracted-1.4.3 --cpus 8
```

The resulting executable is `dist/ultratears`, with its config and data kept beside it. Linux requires a working Vulkan driver that passes the project's hardware checks. Libraries must be available through the system loader or a project-local runtime environment. This PC uses a project-local environment and leaves the system toolchain unchanged.

## Fullscreen and frame rate

`TOTK_FULLSCREEN=1` starts borderless desktop fullscreen; F11 or Alt+Enter toggles it. `TOTK_FPS_LIMIT=60` selects the existing 60 FPS timing patches. The local desktop launcher now defaults to fullscreen and a 60 FPS target. These startup settings require a restart to change in an existing process. Actual frame rate depends on the scene and hardware; presentation fullscreen does not imply native desktop-resolution rendering.

## Controller layout

The default follows Switch button positions. Set `TOTK_CONTROLLER_LAYOUT=xbox-letters` to match physical Xbox A/B/X/Y to the same game letters. Deliberate button or stick input selects that SDL controller instance, including when a wired connection and a wireless receiver appear separately. Device selection cannot establish a missing wireless radio connection or pairing.

## Debug mode

Run with separate session logs and automatic GDB crash capture:

```sh
python3 tools/debug_run.py --game dist/ultratears --log-root ~/.local/share/ultratears/logs
```

The local desktop setup supplies **UltraTears (Debug)**, plus **Debug Mode** and **Open Logs** actions on the normal shortcut. Normal and debug launches share the selected save profile; the launcher prevents simultaneous sessions for that profile.

Each debug session has `session.json`, `debugger.log` and the game's `ultratears.log`. `debug-latest` points to the latest session. The runner enables the game's statistics, preserves expected guarded memory probes, and captures all-thread backtraces for unhandled faults before terminating the failed process. When guest CPU state is available, `guest-state.json` and bounded memory blocks capture nearby guest objects (at most 256 KiB). These blocks are private, may contain game-derived data, and must not be committed or shared. GDB startup scripts and debuginfod downloads are disabled. Without GDB, statistics/logging still work but automatic symbolic crash capture is unavailable. Logs remain local and are not uploaded.

## Validation and source history

Run `tests/local_linux/run.sh` with the build dependencies available. It tests the actual memory-pool implementation, native POSIX resources/signal masks, missing-file errors, Linux guarded-fault recovery, active controller selection, Xbox-letter mapping and diagnostic capture using synthetic fixtures.

The repaired build completed an isolated 2,400-frame headless test using a copy of the imported save. The player actor was present, with no unguarded fault, pool-allocation failure or Vulkan error recorded. Visual correctness, extended play and other GPU/driver combinations still need separate verification.

Local version history preserves the source before and after these changes. Game files, generated game code, saves, keys, binaries, tool downloads and private diagnostic artifacts are excluded from the source snapshots.

## Known visual issue

On Steam Deck, distant smoke can intermittently appear and disappear during play with the 60 FPS target selected. This is a user-reported rendering issue; its cause and relationship to the frame-rate setting have not been established. Investigation is deferred. It should remain listed as an open issue when sharing the Linux changes.
