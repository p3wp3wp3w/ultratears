#!/usr/bin/env python3
"""Install an independent native Steam shortcut; run with Steam fully closed.

Requires ValvePython/vdf (MIT). No game assets or saves are modified.
"""
import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zlib

sys.path.insert(0, str(Path.home() / '.cache/ultratears-setup/vendor'))
import vdf


def replace(mapping, key, value):
    while key in mapping:
        del mapping[key]
    mapping[key] = value


def atomic(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_name(path.name + '.ultratears-new')
    temp.write_bytes(payload)
    os.chmod(temp, 0o600)
    temp.replace(path)


def main():
    if subprocess.run(['pgrep', '-x', 'steam'], capture_output=True).returncode == 0:
        raise SystemExit('Close Steam cleanly before updating its shortcuts.')
    home = Path.home()
    root = home / 'Applications/UltraTears'
    if not (root / 'run.sh').is_file():
        raise SystemExit('Standalone runtime is missing.')
    steam = home / '.local/share/Steam'
    configs = list((steam / 'userdata').glob('*/config'))
    if len(configs) != 1:
        raise SystemExit('Select an account explicitly: expected one Steam user configuration.')
    conf = configs[0]
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    backup = home / '.local/share/UltraTears-backups' / stamp / 'steam-before-install'
    backup.mkdir(parents=True, mode=0o700)
    shortcuts = conf / 'shortcuts.vdf'
    local = conf / 'localconfig.vdf'
    controller = steam / 'steamapps/common/Steam Controller Configs' / conf.parent.name / 'config/configset_controller_neptune.vdf'
    for source, name in ((shortcuts, 'shortcuts.vdf'), (local, 'localconfig.vdf'), (controller, 'configset_controller_neptune.vdf')):
        if source.exists():
            shutil.copy2(source, backup / name)
            os.chmod(backup / name, 0o600)
    data = vdf.binary_loads(shortcuts.read_bytes()) if shortcuts.exists() else {'shortcuts': {}}
    entries = data.setdefault('shortcuts', {})
    exe = '"' + str(root / 'run.sh') + '"'
    name = 'UltraTears'
    unsigned = zlib.crc32((exe + name).encode()) | 0x80000000
    signed = unsigned - 0x100000000
    gameid = (unsigned << 32) | 0x02000000
    existing = [k for k, e in entries.items() if e.get('exe') == exe]
    if len(existing) > 1:
        raise SystemExit('Multiple matching shortcuts; preserved without edits.')
    slot = existing[0] if existing else str(max([int(k) for k in entries] + [-1]) + 1)
    if any(e.get('appid') == signed and k != slot for k, e in entries.items()):
        raise SystemExit('Shortcut app ID collision; preserved without edits.')
    original = {k: e for k, e in entries.items() if k != slot}
    entries[slot] = {
        'appid': signed, 'appname': name, 'exe': exe,
        'StartDir': '"' + str(root) + '"', 'icon': '', 'ShortcutPath': '',
        'LaunchOptions': '', 'IsHidden': 0, 'AllowDesktopConfig': 0,
        'AllowOverlay': 1, 'OpenVR': 0, 'Devkit': 0, 'DevkitGameID': '',
        'DevkitOverrideAppID': 0, 'LastPlayTime': 0,
        'FlatpakAppID': '', 'sortas': '', 'tags': {'0': 'Installed locally'},
    }
    encoded = vdf.binary_dumps(data)
    decoded = vdf.binary_loads(encoded)
    assert {k: e for k, e in decoded['shortcuts'].items() if k != slot} == original

    control = vdf.loads(controller.read_text(encoding='utf-8-sig'), mapper=vdf.VDFDict) if controller.exists() else vdf.VDFDict()
    selections = control.setdefault('controller_config', vdf.VDFDict())
    aliases = (name.lower(), str(unsigned), str(signed))
    template = 'controller_neptune_gamepad_joystick.vdf'
    if not (steam / 'controller_base/templates' / template).is_file():
        raise SystemExit('The Steam Deck gamepad template is missing.')
    for alias in aliases:
        replace(selections, alias, vdf.VDFDict([('template', template)]))
    control_bytes = vdf.dumps(control, pretty=True).encode()
    assert vdf.loads(control_bytes.decode(), mapper=vdf.VDFDict) == control

    localdata = vdf.loads(local.read_text(encoding='utf-8-sig'), mapper=vdf.VDFDict)
    node = localdata
    for key in ('UserLocalConfigStore', 'Software', 'Valve', 'Steam', 'apps', str(unsigned)):
        node = node.setdefault(key, vdf.VDFDict())
    replace(node, 'UseSteamControllerConfig', '1')
    localbytes = vdf.dumps(localdata, pretty=True).encode()
    assert vdf.loads(localbytes.decode(), mapper=vdf.VDFDict) == localdata
    atomic(shortcuts, encoded)
    atomic(controller, control_bytes)
    atomic(local, localbytes)
    desktop = '\n'.join([
        '[Desktop Entry]', 'Type=Application', 'Name=UltraTears',
        'Comment=Play UltraTears', f'Exec=steam steam://rungameid/{gameid}',
        'Icon=input-gaming', 'Terminal=false', 'Categories=Game;',
        'Actions=Debug;Logs;', '', '[Desktop Action Debug]',
        'Name=Debug Mode', f'Exec="{root}/run.sh" --debug', '',
        '[Desktop Action Logs]', 'Name=Open Logs', f'Exec="{root}/run.sh" --logs', '',
    ])
    for path in (home / 'Desktop/UltraTears.desktop', home / '.local/share/applications/UltraTears.desktop'):
        atomic(path, desktop.encode())
        os.chmod(path, 0o755)
    report = {'utc': stamp, 'shortcut_installed': True, 'existing_shortcuts_preserved': len(original),
              'gameid': str(gameid), 'appid': unsigned, 'gamepad_template': template,
              'controller_aliases': len(aliases), 'desktop_launches_through_steam': True,
              'physical_input_verified': False}
    (root / 'steam-install-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
