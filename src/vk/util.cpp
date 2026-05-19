#include "vk/util.h"

namespace vk {

uint32_t find_type(device_t &d, uint32_t bits, VkFlags req) {
    for (uint32_t i = 0; i < d.mem.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (d.mem.memoryTypes[i].propertyFlags & req) == req)
            return i;
    return 0;
}

}
