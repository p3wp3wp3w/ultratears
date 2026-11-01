
#include <assert.h>
#include "../../src/gfx/compat/gfx_compat.h"
#include <sys/resource.h>
#include <sys/ucontext.h>
static sigjmp_buf checkpoint;
static volatile sig_atomic_t signals;
static void handle(int sig) { (void)sig; signals++; siglongjmp(checkpoint, 1); }
int main(void) {
 struct rusage usage; assert(getrusage(RUSAGE_SELF, &usage) == 0);
 struct sigaction action = {0}, previous;
 action.sa_handler = handle; sigemptyset(&action.sa_mask);
 assert(sigaction(SIGUSR1, &action, &previous) == 0);
 for (int round = 0; round < 2; round++) {
  if (sigsetjmp(checkpoint, 1) == 0) { raise(SIGUSR1); assert(!"signal did not jump"); }
  sigset_t blocked; assert(sigprocmask(SIG_SETMASK, NULL, &blocked) == 0);
  assert(!sigismember(&blocked, SIGUSR1));
 }
 assert(signals == 2); assert(sigaction(SIGUSR1, &previous, NULL) == 0);
 return 0;
}
