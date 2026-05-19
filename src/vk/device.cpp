#include "vk/device.h"
#include "log.h"

namespace vk {

bool device_t::create(VkInstance inst) {
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst, &n, nullptr);
    if (!n) { LOGS("no vulkan gpu"); return false; }
    auto pds = std::vector<VkPhysicalDevice>(n);
    vkEnumeratePhysicalDevices(inst, &n, pds.data());
    pd = pds[0];
    vkGetPhysicalDeviceProperties(pd, &props);
    vkGetPhysicalDeviceMemoryProperties(pd, &mem);
    LOG("gpu: %s", props.deviceName);
    LOG("  api: %u.%u.%u",
        VK_API_VERSION_MAJOR(props.apiVersion),
        VK_API_VERSION_MINOR(props.apiVersion),
        VK_API_VERSION_PATCH(props.apiVersion));
    LOG("  driver: %u.%u.%u",
        VK_VERSION_MAJOR(props.driverVersion),
        VK_VERSION_MINOR(props.driverVersion),
        VK_VERSION_PATCH(props.driverVersion));
    LOG("  vendor: 0x%04X device: 0x%04X", props.vendorID, props.deviceID);
    LOG("  memory: %u heaps, %u types",
        mem.memoryHeapCount, mem.memoryTypeCount);
    for (uint32_t i = 0; i < mem.memoryHeapCount; i++)
        LOG("    heap %u: %llu MB %s", i, mem.memoryHeaps[i].size >> 20,
            mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "device local" : "host");

    uint32_t qn = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &qn, nullptr);
    auto qps = std::vector<VkQueueFamilyProperties>(qn);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &qn, qps.data());
    for (qf = 0; qf < qn; qf++) {
        LOG("  queue family %u: %u queues, flags 0x%X",
            qf, qps[qf].queueCount, qps[qf].queueFlags);
        if (qps[qf].queueFlags & VK_QUEUE_GRAPHICS_BIT) break;
    }
    LOG("using queue family %u", qf);

    float prio = 1.0f;
    auto qci = VkDeviceQueueCreateInfo{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = qf;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    auto dci = VkDeviceCreateInfo{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    if (vkCreateDevice(pd, &dci, nullptr, &dev) != VK_SUCCESS)
        return false;
    vkGetDeviceQueue(dev, qf, 0, &queue);
    LOG("logical device created");

    auto cpci = VkCommandPoolCreateInfo{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = qf;
    vkCreateCommandPool(dev, &cpci, nullptr, &pool);
    return true;
}

void device_t::destroy() {
    vkDestroyCommandPool(dev, pool, nullptr);
    vkDestroyDevice(dev, nullptr);
    LOGS("vulkan device destroyed");
}

}