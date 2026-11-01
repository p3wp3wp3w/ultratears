
#include <assert.h>
#include "../../src/gfx/nvn/totk_vpool_win.c"
void plat_fatal_notice(void) { abort(); }
int main(void) {
 const size_t n = 111u * 1024u * 1024u;
 uint8_t original;
 pool_t physical = {.magic = POOL_MAGIC, .flags = F_PHYSICAL | F_CPU_NO_ACCESS, .mem = &original, .size = n};
 pool_t virtual = {.magic = POOL_MAGIC, .flags = F_VIRTUAL, .size = n};
 assert(totk_vpool_init(&physical));
 assert(totk_vpool_init(&virtual));
 uintptr_t physical_address = (uintptr_t)physical.mem, virtual_address = (uintptr_t)virtual.mem;
 uint64_t bytes, pools; totk_vpool_host_stats(&bytes, &pools); assert(bytes == n && pools == 1);
 physical.mem[123] = 42;
 totk_mapping_request request = {.physical_pool = (const NVNmemoryPool *)&physical, .size = TOTK_VPOOL_PAGE};
 assert(totk_vpool_map(&virtual, 1, &request));
 uint8_t value = 0; assert(totk_vpool_gather(virtual_address + 123, 1, &value)); assert(value == 42);
 uintptr_t backing = 0; assert(totk_vpool_backing(virtual_address + 123, 1, &backing)); assert(backing == physical_address + 123);
 request.physical_pool = NULL; assert(totk_vpool_map(&virtual, 1, &request));
 value = 42; assert(totk_vpool_gather(virtual_address + 123, 1, &value)); assert(value == 0);
 assert(totk_vpool_fini(&virtual)); assert(totk_vpool_fini(&physical));
 unsigned char state;
 errno = 0; assert(mincore((void *)virtual_address, 4096, &state) == -1 && errno == ENOMEM);
 errno = 0; assert(mincore((void *)physical_address, 4096, &state) == -1 && errno == ENOMEM);
 totk_vpool_host_stats(&bytes, &pools); assert(bytes == 0 && pools == 0);
 assert(VirtualAlloc(NULL, 0, MEM_RESERVE, PAGE_NOACCESS) == NULL); assert(GetLastError() == EINVAL);
 return 0;
}
