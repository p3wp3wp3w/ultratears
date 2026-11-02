#!/usr/bin/env python3
"""Run a local diagnostic session; capture real faults without crash-dialog loops."""
import argparse
import datetime as dt
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def debugger_script(state_directory=None):
    # Addresses and backtraces stay local. Expected guarded graphics probes must
    # reach the game's signal handler, rather than being reported as crashes.
    state_capture = ''
    if state_directory is not None:
        module = Path(__file__).with_name('debug_state.py')
        state_capture = '''python
try:
    import importlib.util
    spec = importlib.util.spec_from_file_location('ultrs_debug_state', %r)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.capture(%r)
except Exception:
    print('Guest-state capture unavailable; retaining the standard backtrace')
end
''' % (str(module), str(state_directory))
    return """set pagination off
set confirm off
set debuginfod enabled off
set disable-randomization off
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
catch signal SIGSEGV SIGBUS SIGABRT SIGILL SIGFPE
commands
silent
python
import gdb
expected = False
try:
    signum = int(gdb.parse_and_eval('$_siginfo.si_signo'))
    expected = signum in (7, 11) and bool(int(gdb.parse_and_eval('runtime_fault_guard')))
except gdb.error:
    pass
gdb.set_convenience_variable('ultrs_expected_probe', gdb.Value(expected))
end
if $ultrs_expected_probe
continue
end
printf "\\nULTRATEARS_CRASH_CAPTURED\\n"
printf "signal %d address %p\\n", $_siginfo.si_signo, $_siginfo._sifields._sigfault.si_addr
thread apply all bt 12
info registers
quit 1
end
run
python
try:
    code = int(gdb.parse_and_eval('$_exitcode'))
except (gdb.error, ValueError):
    code = 1
gdb.execute('quit %d' % code)
end
""".replace('thread apply all bt 12', state_capture + 'thread apply all bt 12')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--log-root', type=Path)
    parser.add_argument('--no-notify', action='store_true')
    parser.add_argument('arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    game = args.game.expanduser().resolve()
    if not game.is_file() or not os.access(game, os.X_OK):
        parser.error('game executable is missing or is not executable')
    arguments = args.arguments[1:] if args.arguments[:1] == ['--'] else args.arguments
    now = dt.datetime.now(dt.timezone.utc)
    log_root = args.log_root or Path(os.environ.get('LARECOMP_DATA_ROOT', Path.home() / '.local/share/UltraTears/logs'))
    log_root = log_root.expanduser().resolve()
    session = log_root / ('debug-' + now.strftime('%Y%m%dT%H%M%S.%fZ') + '-' + str(os.getpid()))
    session.mkdir(parents=True, mode=0o700)
    latest = log_root / 'debug-latest'
    if latest.is_symlink():
        latest.unlink()
    if not latest.exists():
        latest.symlink_to(session.name, target_is_directory=True)
    env = dict(os.environ, TOTK_STATS='1', LARECOMP_DATA_ROOT=str(session))
    debugger = shutil.which('gdb')
    command = [str(game), '--debug', *arguments]
    if debugger:
        script = session / 'commands.gdb'
        script.write_text(debugger_script(session))
        command = [debugger, '--nx', '--batch', '-x', str(script), '--args', *command]
    metadata = {'started_utc': now.isoformat(), 'mode': 'debug',
                'debugger': bool(debugger), 'game': str(game), 'session': str(session),
                'state': 'running'}
    metadata_path = session / 'session.json'
    metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
    start = time.monotonic()
    with (session / 'debugger.log').open('w') as output:
        result = subprocess.run(command, cwd=game.parent, env=env,
                                stdin=subprocess.DEVNULL, stdout=output, stderr=output)
    trace = (session / 'debugger.log').read_text(errors='replace')
    crashed = 'ULTRATEARS_CRASH_CAPTURED' in trace or result.returncode < 0
    metadata.update(state='crashed' if crashed else 'stopped', exit_code=result.returncode,
                    crash_captured=crashed, duration_seconds=round(time.monotonic() - start, 3),
                    finished_utc=dt.datetime.now(dt.timezone.utc).isoformat())
    metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps({'session': str(session), 'crash_captured': crashed, 'exit_code': result.returncode}))
    if crashed and not args.no_notify:
        notify = shutil.which('notify-send')
        if notify:
            subprocess.run([notify, 'UltraTears debug report saved', str(session)], check=False)
    return result.returncode if result.returncode >= 0 else 128 - result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
