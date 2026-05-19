#pragma once
#include "vk/types.h"

namespace vk {

struct device_t {
    VkPhysicalDevice pd = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t qf = 0;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties props = {};
    VkPhysicalDeviceMemoryProperties mem = {};

    bool create(VkInstance inst);
    void destroy();
};

}