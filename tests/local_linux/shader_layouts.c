#define _GNU_SOURCE
#include <assert.h>
#include <sched.h>
#include <unistd.h>
#include "../../src/gfx/vkb_shader.c"

__typeof__(vk) vk;
__typeof__(vkc) vkc;
enum { READERS = 12 };
static _Atomic int first_published, readers_started, sets_created, pipelines_created, db_ready;
int vkb_db_on(void) { return 1; }
void vkb_db_layouts(const VkDescriptorSetLayout layouts[VKB_LAYOUTS]) {
    for (int i = 0; i < VKB_LAYOUTS; i++) assert(layouts[i]);
    atomic_store(&db_ready, 1);
}
int vk_check(VkResult result, const char *what) { (void)what; assert(result == VK_SUCCESS); return 1; }
static VKAPI_ATTR VkResult VKAPI_CALL make_set(VkDevice device, const VkDescriptorSetLayoutCreateInfo *ci,
                                             const VkAllocationCallbacks *alloc, VkDescriptorSetLayout *out) {
    (void)device; (void)ci; (void)alloc;
    int id = atomic_fetch_add(&sets_created, 1);
    *out = (VkDescriptorSetLayout)(uintptr_t)(id + 1);
    if (id == 0) {
        atomic_store(&first_published, 1);
        while (atomic_load(&readers_started) != READERS) sched_yield();
        usleep(20000);   // readers enter while the other layouts are still absent
    }
    return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL make_pipeline(VkDevice device, const VkPipelineLayoutCreateInfo *ci,
                                                  const VkAllocationCallbacks *alloc, VkPipelineLayout *out) {
    (void)device; (void)alloc;
    for (unsigned i = 0; i < ci->setLayoutCount; i++) assert(ci->pSetLayouts[i]);
    *out = (VkPipelineLayout)(uintptr_t)(atomic_fetch_add(&pipelines_created, 1) + 1);
    return VK_SUCCESS;
}
static void *owner(void *arg) { (void)arg; assert(vkb_set_layout(0)); return NULL; }
static void *reader(void *arg) {
    int kind = (int)(intptr_t)arg;
    while (!atomic_load(&first_published)) sched_yield();
    atomic_fetch_add(&readers_started, 1);
    if (kind % 3 == 0) {
        VkDescriptorSetLayout layouts[2];
        assert(vkb_set_layouts(0, layouts) == 2 && layouts[0] && layouts[1]);
    } else if (kind % 3 == 1) assert(vkb_set_layout(1));
    else assert(vkb_pipeline_layout(0));
    assert(atomic_load(&db_ready));
    return NULL;
}
int main(void) {
    vk.vkCreateDescriptorSetLayout = make_set;
    vk.vkCreatePipelineLayout = make_pipeline;
    pthread_t init, readers[READERS];
    assert(!pthread_create(&init, NULL, owner, NULL));
    for (int i = 0; i < READERS; i++) assert(!pthread_create(&readers[i], NULL, reader, (void *)(intptr_t)i));
    assert(!pthread_join(init, NULL));
    for (int i = 0; i < READERS; i++) assert(!pthread_join(readers[i], NULL));
    assert(atomic_load(&sets_created) == VKB_LAYOUTS && atomic_load(&pipelines_created) == 2);
    VkDescriptorSetLayout layouts[2];
    assert(vkb_set_layouts(1, layouts) == 1 && layouts[0]);
    return 0;
}
