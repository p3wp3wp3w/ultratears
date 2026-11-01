Linux regression checks

Run tests/local_linux/run.sh from a source checkout with Clang and SDL3 development headers/libraries available. For this PC, source the project-local environment.sh first. Temporary test executables are removed automatically.

vpool.c: includes the actual NVN pool implementation, with the same graphics compatibility-header search path as the game. Allocates the previously failing 111 MiB pools, maps a physical page into a virtual pool, gathers its bytes, unmaps to zeros, frees both mappings and checks that host stats and mincore agree. Verifies errno on a failed zero-length request.
posix.c: verifies native getrusage and two signal-mask-restoring sigsetjmp/siglongjmp cycles through the graphics compatibility includes.
romfs.c: verifies a nonexistent game resource returns path-not-found without inventing a handle or changing the handle table. This test was run against the pre-fix implementation and failed as expected.
fault_guard.c: includes the actual Linux platform handler and proves two real reads from a PROT_NONE page recover through an explicitly installed thread-local fault guard, leaving SIGSEGV unblocked. Unguarded faults remain fatal.
controller.c: includes the actual SDL controller implementation and creates two virtual device instances. Checks selection switches from an idle receiver to the second controller and verifies all four Xbox-letter button mappings.

These are code-level regressions, not proof of a full game playthrough, correct graphics in every scene, wireless pairing, or future GPU compatibility. No game data or external services are required by these tests.

debug_runner.py: runs three synthetic programs under the actual diagnostic wrapper and GDB: clean exit, an unhandled segmentation fault with captured main backtrace, and two intentionally guarded faults that must resume without being classified as crashes. The shell runner includes this test when GDB is installed. No game is launched by these regression programs.
