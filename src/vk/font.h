#pragma once
#include "vk/types.h"
#include "vk/device.h"

namespace vk {

struct font_t {
    VkImage img = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;

    bool create(device_t &d);
    void destroy(device_t &d);
};

}