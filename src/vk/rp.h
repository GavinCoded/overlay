#pragma once
#include "vk/types.h"
#include "vk/device.h"

namespace vk {

struct rp_t {
    VkRenderPass rp = VK_NULL_HANDLE;
    VkFramebuffer fb = VK_NULL_HANDLE;

    bool create(device_t &d, VkImageView iv, int w, int h);
    void destroy(device_t &d);
};

}
