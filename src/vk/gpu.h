#pragma once
#include "vk/types.h"
#include "vk/device.h"

namespace vk {

struct gpu_t {
    VkImage img = VK_NULL_HANDLE;
    VkDeviceMemory img_mem = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkBuffer host_buf = VK_NULL_HANDLE;
    VkDeviceMemory host_mem = VK_NULL_HANDLE;
    void *mapped = nullptr;
    int w = 0, h = 0;

    bool create(device_t &d, int width, int height);
    void destroy(device_t &d);
};

}