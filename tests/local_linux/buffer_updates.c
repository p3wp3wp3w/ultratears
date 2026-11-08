#define _GNU_SOURCE
#include <assert.h>
#include "../../src/gfx/backend_vk.c"

__typeof__(vk) vk;
__typeof__(vkc) vkc;
static unsigned char upload[3 * 65536], expected[sizeof upload];
static size_t used, copied, updated, skipped;
static int mapped = 1;
int vkb_gpu_addr(uint64_t addr, uint64_t n, VkBuffer *buf, VkDeviceSize *off) {
    (void)n;
    *buf = (VkBuffer)(uintptr_t)2;
    *off = addr;
    return mapped;
}
void *vkb_ring_alloc(VkDeviceSize n, VkDeviceSize align, VkBuffer *buf, VkDeviceSize *off) {
    assert(align == 4 && n <= 65536 && used + n <= sizeof upload);
    *buf = (VkBuffer)(uintptr_t)1;
    *off = used;
    void *p = upload + used;
    used += n;
    return p;
}
void vkb_audit(int kind, const char *key) { (void)kind; (void)key; skipped++; }
static VKAPI_ATTR void VKAPI_CALL copy(VkCommandBuffer cb, VkBuffer src, VkBuffer dst,
                                      uint32_t count, const VkBufferCopy *regions) {
    assert(cb == (VkCommandBuffer)(uintptr_t)3);
    assert(src == (VkBuffer)(uintptr_t)1 && dst == (VkBuffer)(uintptr_t)2 && count == 1);
    const VkBufferCopy *r = regions;
    assert(r->srcOffset == copied && r->dstOffset == 128 + copied);
    assert(r->size <= 65536 && !memcmp(upload + r->srcOffset, expected + copied, r->size));
    copied += r->size;
}
static VKAPI_ATTR void VKAPI_CALL update(VkCommandBuffer cb, VkBuffer dst, VkDeviceSize off,
                                        VkDeviceSize n, const void *data) {
    (void)cb; (void)dst;
    assert(off == 128 + updated && n <= 65536);
    assert(!memcmp(data, expected + updated, n));
    updated += n;
}
int main(void) {
    vk.vkCmdCopyBuffer = copy;
    vk.vkCmdUpdateBuffer = update;
    for (size_t i = 0; i < sizeof expected; i++) expected[i] = (unsigned char)(i * 37);
    vkc.props.vendorID = 0x1002;
    ub_write((VkCommandBuffer)(uintptr_t)3, 128, 64, expected);
    assert(copied == 64 && used == 64 && updated == 0);
    unsigned char saved = upload[0];
    expected[0] ^= 0xff;
    assert(upload[0] == saved);   // source reuse cannot change the pending upload
    expected[0] ^= 0xff;
    used = copied = 0;
    ub_write((VkCommandBuffer)(uintptr_t)3, 128, 131076, expected);
    assert(copied == 131076 && used == 131076 && updated == 0);
    ub_write((VkCommandBuffer)(uintptr_t)3, 129, 64, expected);
    assert(skipped == 1);
    mapped = 0;
    ub_write((VkCommandBuffer)(uintptr_t)3, 128, 64, expected);
    assert(copied == 131076 && used == 131076);
    mapped = 1;
    vkc.props.vendorID = 0x10de;
    ub_write((VkCommandBuffer)(uintptr_t)3, 128, 131076, expected);
    assert(updated == 131076 && copied == 131076);
    return 0;
}
