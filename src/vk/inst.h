#pragma once
#include "vk/types.h"

namespace vk {

struct inst_t {
    VkInstance v = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT d = VK_NULL_HANDLE;
    bool create();
    void destroy();
};

}