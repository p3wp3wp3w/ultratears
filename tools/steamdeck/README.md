# Standalone Steam Deck integration

These optional scripts configure a native UltraTears build in
`~/Applications/UltraTears`, independent of EmuDeck. They contain no game data.
Build from your own supported, extracted game dump using the project setup
instructions. Check the target system's library and CPU requirements; a binary
built on another distribution may require a newer glibc than the target has.

Copy the resulting runtime and its required data to that directory, then place
`run.sh` and the example `ultratears.cfg` beside the executable. Do not copy
another machine's saves over existing Deck progress or reuse a different GPU's
compiled driver cache. Retain the target machine's original saves and make a
verified backup before any save import. Save import is deliberately separate
from the Steam installer.

The launcher uses its own `userdata` profile, logs and compiled shader cache;
it prevents simultaneous runs sharing that profile. Defaults are fullscreen,
handheld operation mode, letter-matched ABXY, rumble, and SDL X11/Xwayland.
It currently selects a **60 FPS target for testing**; this does not establish
sustained 60 FPS performance. For a 30 FPS run, set `TOTK_FPS_LIMIT=30` before
launching or change that default in the launcher. Internal dynamic resolution
remains controlled by the game. The fullscreen output follows the display and
preserves the game's aspect ratio.

## Steam Library and Desktop shortcut

Close Steam normally first. The installer expects exactly one Steam user
configuration and preserves existing shortcuts. It backs up the relevant
configuration files under `~/.local/share/UltraTears-backups/`, then adds an
UltraTears shortcut and selects Valve's Steam Deck Gamepad template. It uses
ValvePython/vdf; one option is an isolated environment:

```sh
python3 -m venv ~/.cache/ultratears-setup/venv
~/.cache/ultratears-setup/venv/bin/python -m pip install vdf==3.4
~/.cache/ultratears-setup/venv/bin/python tools/steamdeck/install-steam.py
```

Restart Steam after installation and launch UltraTears from its Library entry.
The Desktop shortcut also launches through Steam to activate the gamepad
configuration. Test the built-in controls in SteamOS Gaming Mode, including
sticks and physical ABXY correspondence. Do not infer working input solely
from controller enumeration. Existing emulator profiles are not modified.

`run.sh --debug` uses `tools/debug_run.py` from the runtime directory. Copy
that helper there if debug mode is wanted. Keep debug symbols, or a separate
GNU debuglink symbol file, for symbolic backtraces. `run.sh --logs` opens the
profile's log directory. Direct debug launch does not by itself establish the
Steam shortcut's controller context.

## Scope and known limitation

The Deck configuration was tested on SteamOS with SDL3 and RADV. A disposable
save test reached 2,100 frames without fatal signals or Vulkan errors; Gaming
Mode launch and user-confirmed controller operation subsequently worked.
This is not validation of every driver, distribution or game scene.
Distant smoke intermittently appearing/disappearing remains an open,
user-reported visual issue; its cause is unconfirmed.

Share these scripts, runtime source patches, tests and documentation under the
repository's licenses. Exclude game dumps/assets, generated game code, extracted
fonts, converted shader/texture caches, saves, keys, firmware and private logs.
