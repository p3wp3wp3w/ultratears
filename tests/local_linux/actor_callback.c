#define _GNU_SOURCE
#include <assert.h>
#include "../../src/host/guest.c"

void (*rt_fault_extra)(void);
uint32_t runtime_thread_current_handle(void) { return 0; }
const char *loader_symbol_at(uint64_t addr, uint64_t *offset) { (void)addr; *offset = 0; return NULL; }
void hle_override(uint64_t addr, HostFn fn, const char *name) { (void)addr; (void)fn; (void)name; }
void *hle_alloc(uint64_t n) { return calloc(1, n); }
static void live_callback(Cpu *c) { (void)c; }
GuestFn dispatch_lookup(uint64_t target) {
    return target == MAIN_BASE + 0x1000 ? live_callback : NULL;
}
int main(void) {
    assert(totk_actor_callback_ok(MAIN_BASE + 0x1000));
    assert(!totk_actor_callback_ok(0));
    // A recycled actor/list node is guest data, never a callable entry.
    uint64_t node = 0x1000010000ULL;
    assert(!totk_actor_callback_ok(node));
    assert(!totk_actor_callback_ok(MAIN_BASE + 0x1004));
    assert(totk_actor_callback_ok(MAIN_BASE + 0x1000));
    return 0;
}
