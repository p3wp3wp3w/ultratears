
#include <assert.h>
#include "../../src/host/fs.c"
uint64_t runtime_now_ns(void) { return 0; }
void runtime_fail(const char *why) { (void)why; abort(); }
int main(void) {
 strcpy(mount_name, "rom");
 uint64_t handle = 0xdeadbeef;
 assert(romfs_open(&handle, "rom:/missing-model.bfres.zs", OPEN_READ) == R_PATH_NOT_FOUND);
 assert(handle == 0xdeadbeef);
 for (unsigned i = 0; i < NROM; i++) assert(rom_handles[i] == 0);
 return 0;
}
