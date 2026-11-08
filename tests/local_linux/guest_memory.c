#define _GNU_SOURCE
#include <assert.h>
#include <sys/mman.h>

#include "../../src/host/svc.c"

Module g_modules[MAX_MODULES];
int g_nmodules;
uint64_t totk_heap_extra_bytes(void) { return 0; }

_Noreturn void runtime_fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    abort();
}

static void expect_zero(const unsigned char *page) {
    for (unsigned i = 0; i < 4096; i++) assert(page[i] == 0);
}

int main(void) {
    unsigned char *pages = mmap((void *)ALIAS_START, 3 * 4096,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    assert(pages == (void *)ALIAS_START);
    assert(alias_map(0, ALIAS_START, 3 * 4096) == 0);
    memset(pages, 0x31, 4096);
    memset(pages + 4096, 0x72, 4096);
    memset(pages + 8192, 0x93, 4096);
    assert(alias_map(1, ALIAS_START + 4096, 4096) == 0);
    assert(alias_bytes == 8192);
    // A partly overlapping remap preserves live pages and zeroes the fresh one.
    assert(alias_map(0, ALIAS_START, 3 * 4096) == 0);
    assert(alias_bytes == 3 * 4096);
    expect_zero(pages + 4096);
    for (unsigned i = 0; i < 4096; i++) {
        assert(pages[i] == 0x31);
        assert(pages[8192 + i] == 0x93);
    }
    // Discarding twice must leave the guest's reserved mapping usable.
    decommit(ALIAS_START, 3 * 4096);
    decommit(ALIAS_START, 3 * 4096);
    for (unsigned i = 0; i < 3; i++) expect_zero(pages + i * 4096);
    memset(pages, 0x42, 3 * 4096);
    assert(munmap(pages, 3 * 4096) == 0);
    return 0;
}
