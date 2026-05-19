#pragma once
#include "vk/types.h"
#include "vk/device.h"

namespace vk {

uint32_t find_type(device_t &d, uint32_t bits, VkFlags req);

}
