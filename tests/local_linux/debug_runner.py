"""Exercise diagnostics with synthetic programs, without game data or a GUI."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    if not shutil.which('gdb'):
        raise SystemExit('GDB is required for debugger regression tests')
    fixtures = {
        'normal': 'int main(void) { return 0; }',
        'crash': 'int main(void) { *(volatile int *)0 = 1; return 0; }',
        'guarded': r'''
#define _GNU_SOURCE
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdlib.h>
__thread sigjmp_buf *runtime_fault_guard;
static void handler(int sig) {
    (void)sig;
    sigjmp_buf *checkpoint = runtime_fault_guard;
    runtime_fault_guard = NULL;
    siglongjmp(*checkpoint, 1);
}
int main(void) {
    struct sigaction action = {0}; action.sa_handler = handler;
    assert(sigaction(SIGSEGV, &action, NULL) == 0);
    void *page = mmap(NULL, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(page != MAP_FAILED);
    for (int i = 0; i < 2; i++) {
        sigjmp_buf checkpoint;
        if (sigsetjmp(checkpoint, 1) == 0) {
            runtime_fault_guard = &checkpoint;
            volatile char value = *(volatile char *)page; (void)value;
            abort();
        }
        assert(runtime_fault_guard == NULL);
    }
    return 0;
}
''',
    }
    with tempfile.TemporaryDirectory(prefix='ultratears-debug-test-') as directory:
        temp = Path(directory)
        for name, source in fixtures.items():
            c = temp / (name + '.c')
            c.write_text(source)
            binary = temp / name
            subprocess.run([os.environ.get('CC', 'clang'), '-O0', '-g', str(c), '-o', str(binary)], check=True)
            run = subprocess.run([sys.executable, str(ROOT / 'tools/debug_run.py'),
                                  '--game', str(binary), '--log-root', str(temp / 'logs'),
                                  '--no-notify'], capture_output=True, text=True, timeout=30)
            report = json.loads(run.stdout)
            session = Path(report['session'])
            metadata = json.loads((session / 'session.json').read_text())
            assert metadata['debugger']
            assert metadata['state'] != 'running'
            assert (temp / 'logs/debug-latest').resolve() == session
            if name == 'crash':
                assert run.returncode != 0 and metadata['crash_captured']
                assert 'main' in (session / 'debugger.log').read_text()
            else:
                assert run.returncode == 0 and not metadata['crash_captured'], run.stderr
    print('Debug runner: normal exit, real crash trace, and guarded-fault recovery passed.')


if __name__ == '__main__':
    main()
