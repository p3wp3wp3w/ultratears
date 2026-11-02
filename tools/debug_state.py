"""Bounded, local-only guest state capture, loaded inside GDB after a fault."""
import json
from pathlib import Path

import gdb


def capture(directory):
    directory = Path(directory)
    report = {'schema': 1, 'registers': {}, 'blocks': [], 'errors': [],
              'byte_limit': 256 * 1024}
    frame = gdb.selected_frame()
    cpu = None
    while frame:
        try:
            candidate = frame.read_var('c')
            candidate.dereference()['x']
            cpu = candidate.dereference()
            break
        except (gdb.error, ValueError):
            frame = frame.older()
    if cpu is None:
        report['errors'].append('No readable guest CPU in the faulting call chain')
    else:
        inferior = gdb.selected_inferior()
        registers = report['registers']
        for index in range(31):
            registers['x%d' % index] = int(cpu['x'][index])
        for name in ('sp', 'pc', 'nzcv', 'fpcr', 'fpsr'):
            registers[name] = int(cpu[name])
        # Guest addresses are also host addresses. Never follow host pointers,
        # and bound both traversal and bytes regardless of corrupted data.
        pending = [(value, 0) for value in registers.values()]
        seen = set()
        total = 0
        while pending and total < report['byte_limit'] and len(seen) < 128:
            address, depth = pending.pop(0)
            if not 0x800000000 <= address < 0x8000000000:
                continue
            start = address & ~0xff
            if start in seen:
                continue
            seen.add(start)
            size = min(4096 - (start & 4095), report['byte_limit'] - total)
            try:
                data = bytes(inferior.read_memory(start, size))
            except gdb.error:
                continue
            name = 'guest-block-%03d.bin' % len(report['blocks'])
            (directory / name).write_bytes(data)
            report['blocks'].append({'address': start, 'size': size,
                                     'file': name, 'depth': depth})
            total += size
            if depth < 2:
                pending.extend((int.from_bytes(data[i:i + 8], 'little'), depth + 1)
                               for i in range(0, len(data) - 7, 8))
        report['captured_bytes'] = total
    (directory / 'guest-state.json').write_text(json.dumps(report, indent=2) + '\n')
