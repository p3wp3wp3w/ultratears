#define _GNU_SOURCE
#include <assert.h>
#include "../../src/gfx/vkb_draw.c"

__typeof__(vk) vk;
__typeof__(vkc) vkc;
static int binds, inputs;
void plat_fatal_notice(void) { abort(); }
uint64_t mtl_last_fence(void) { return 0; }
int vkb_worker(void) { return 0; }
int vkb_pool_addr(uint64_t a, uint64_t n, VkBuffer *b, VkDeviceSize *o) { (void)a; (void)n; (void)b; (void)o; return 0; }
uint64_t vkb_pool_room(uint64_t a) { (void)a; return 0; }
int vkb_span_addr(uint64_t a, uint64_t n, VkBuffer *b, VkDeviceSize *o) { (void)a; (void)n; (void)b; (void)o; return 0; }
VkFormat vkb_format(int f, VkImageAspectFlags *b, int *c) { (void)f; (void)b; (void)c; return VK_FORMAT_UNDEFINED; }
uint64_t vkb_frame(void) { return 0; }
void vkb_audit(int reason, const char *key) { (void)reason; (void)key; }
static VKAPI_ATTR void VKAPI_CALL properties(VkPhysicalDevice device, VkFormat f, VkFormatProperties *p) {
    (void)device; (void)f; memset(p, 0, sizeof *p); p->bufferFeatures = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
}
static VKAPI_ATTR void VKAPI_CALL bind_buffers(VkCommandBuffer cb, uint32_t first, uint32_t count,
        const VkBuffer *buf, const VkDeviceSize *off, const VkDeviceSize *size, const VkDeviceSize *stride) {
    (void)cb; (void)buf; (void)off; (void)size;
    assert(first == 0 && count == 1);
    if (vkc.has_vertex_input_ds) assert(!stride);
    else assert(stride && *stride == 6);
    binds++;
}
static VKAPI_ATTR void VKAPI_CALL set_input(VkCommandBuffer cb, uint32_t nb,
        const VkVertexInputBindingDescription2EXT *b, uint32_t na, const VkVertexInputAttributeDescription2EXT *a) {
    (void)cb; (void)a; assert(nb == 1 && na == 1 && b[0].stride == 6); inputs++;
}
int main(void) {
    big_init();
    vk.vkGetPhysicalDeviceFormatProperties = properties;
    vk.vkCmdBindVertexBuffers2 = bind_buffers;
    vk.vkCmdSetVertexInputEXT = set_input;
    S.nvattr = 1;
    va_st attribute = {.fmt = 0x1d};
    memcpy(&S.vattr[0], &attribute, sizeof attribute);
    S.vstream[0].stride = 6;
    for (int native = 0; native < 2; native++) {
        vkc.has_vertex_input_ds = native;
        vkb_rec_begin();
        set_vertex_input((VkCommandBuffer)(uintptr_t)1, 0);
    }
    assert(binds == 2 && inputs == 2);
    return 0;
}
