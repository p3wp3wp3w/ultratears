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
        'guest_crash': r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
#include "cpu.h"
__attribute__((noinline)) void guest_fault(Cpu *c) {
    c->x[0] = 0xfffffff3;
    *(volatile int *)0 = (int)c->x[0];
}
int main(void) {
    uint64_t *object = mmap((void *)0x810000000ULL, 4096,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    assert(object == (void *)0x810000000ULL);
    object[0] = 0x12345678;
    object[1] = (uint64_t)object;
    Cpu c = {0};
    c.x[21] = (uint64_t)object;
    c.x[22] = 0xffffffffffffffffULL;
    guest_fault(&c);
    return 0;
}
''',
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
            subprocess.run([os.environ.get('CC', 'clang'), '-O0', '-g',
                            '-I' + str(ROOT / 'src/runtime'), str(c), '-o', str(binary)], check=True)
            run = subprocess.run([sys.executable, str(ROOT / 'tools/debug_run.py'),
                                  '--game', str(binary), '--log-root', str(temp / 'logs'),
                                  '--no-notify'], capture_output=True, text=True, timeout=30)
            report = json.loads(run.stdout)
            session = Path(report['session'])
            metadata = json.loads((session / 'session.json').read_text())
            assert metadata['debugger']
            assert metadata['state'] != 'running'
            assert (temp / 'logs/debug-latest').resolve() == session
            if name in ('crash', 'guest_crash'):
                assert run.returncode != 0 and metadata['crash_captured']
                assert 'main' in (session / 'debugger.log').read_text()
                state = json.loads((session / 'guest-state.json').read_text())
                if name == 'guest_crash':
                    assert state['registers']['x0'] == 0xfffffff3
                    assert state['registers']['x21'] == 0x810000000
                    assert 0 < state['captured_bytes'] <= state['byte_limit']
                    assert len(state['blocks']) == 1  # cycles and invalid pointers are bounded
                    data = (session / state['blocks'][0]['file']).read_bytes()
                    assert int.from_bytes(data[:8], 'little') == 0x12345678
            else:
                assert run.returncode == 0 and not metadata['crash_captured'], run.stderr
    print('Debug runner: normal exit, real crash trace, and guarded-fault recovery passed.')


if __name__ == '__main__':
    main()
