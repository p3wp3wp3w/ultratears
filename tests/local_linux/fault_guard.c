
#define _GNU_SOURCE
#include <assert.h>
#include "../../src/runtime/platform/linux.c"
__thread sigjmp_buf *runtime_fault_guard;
bool rt_fault_watch(uint64_t address, uint64_t pc) { (void)address; (void)pc; return false; }
void rt_fault_report(const char *what, uint64_t address, uint64_t pc, uint64_t lr) {
 (void)what; (void)address; (void)pc; (void)lr; abort();
}
int main(void) {
 plat_crash_handler_install();
 void *page = mmap(NULL, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); assert(page != MAP_FAILED);
 for (int round = 0; round < 2; round++) {
  sigjmp_buf checkpoint;
  if (sigsetjmp(checkpoint, 1) == 0) {
   runtime_fault_guard = &checkpoint;
   volatile unsigned char value = *(volatile unsigned char *)page; (void)value;
   assert(!"protected read unexpectedly succeeded");
  }
  assert(runtime_fault_guard == NULL);
  sigset_t blocked; assert(sigprocmask(SIG_SETMASK, NULL, &blocked) == 0);
  assert(!sigismember(&blocked, SIGSEGV));
 }
 assert(munmap(page, 4096) == 0);
 return 0;
}
