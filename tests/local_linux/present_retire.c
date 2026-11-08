#include <assert.h>
#include "../../src/vk/present.c"

__typeof__(vk) vk;
__typeof__(vkc) vkc;
static int locked, idle, waits, destroyed;
static VkResult wait_result = VK_SUCCESS;
void vk_queue_lock(void) { assert(!locked); locked = 1; }
void vk_queue_unlock(void) { assert(locked); locked = 0; }
int vk_check(VkResult result, const char *what) { (void)what; return result == VK_SUCCESS; }
static VKAPI_ATTR VkResult VKAPI_CALL wait_idle(VkQueue queue) {
    (void)queue;
    assert(locked);
    waits++;
    idle = wait_result == VK_SUCCESS;
    return wait_result;
}
static VKAPI_ATTR void VKAPI_CALL destroy_semaphore(VkDevice device, VkSemaphore sem, const VkAllocationCallbacks *alloc) {
    (void)device; (void)sem; (void)alloc;
    assert(idle && !locked);
    destroyed++;
}
static VKAPI_ATTR void VKAPI_CALL destroy_chain(VkDevice device, VkSwapchainKHR chain, const VkAllocationCallbacks *alloc) {
    (void)device; (void)chain; (void)alloc;
    assert(idle && destroyed == 2);
}
int main(void) {
    vk.vkQueueWaitIdle = wait_idle;
    vk.vkDestroySemaphore = destroy_semaphore;
    vk.vkDestroySwapchainKHR = destroy_chain;
    assert(retire_swapchain() && waits == 0);
    swapchain = (VkSwapchainKHR)(uintptr_t)1;
    nimages = 2;
    wait_result = VK_ERROR_DEVICE_LOST;
    assert(!retire_swapchain() && !locked && destroyed == 0 && nimages == 2 && swapchain);
    wait_result = VK_SUCCESS;
    assert(retire_swapchain() && waits == 2 && destroyed == 2 && !nimages && !swapchain);
    return 0;
}
