#include "vk/gpu.h"
#include "vk/util.h"
#include "log.h"

namespace vk {

bool gpu_t::create(device_t &d, int width, int height) {
    w = width; h = height;
    auto ci = VkImageCreateInfo{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = VK_FORMAT_B8G8R8A8_UNORM;
    ci.extent = {(uint32_t)width, (uint32_t)height, 1};
    ci.mipLevels = 1; ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(d.dev, &ci, nullptr, &img) != VK_SUCCESS)
        return false;

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(d.dev, img, &mr);
    auto ai = VkMemoryAllocateInfo{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = mr.size;
    ai.memoryTypeIndex = find_type(d, mr.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(d.dev, &ai, nullptr, &img_mem) != VK_SUCCESS)
        return false;
    vkBindImageMemory(d.dev, img, img_mem, 0);

    auto ivci = VkImageViewCreateInfo{};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = img;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_B8G8R8A8_UNORM;
    ivci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(d.dev, &ivci, nullptr, &view);

    auto bci = VkBufferCreateInfo{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = (VkDeviceSize)width * height * 4;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(d.dev, &bci, nullptr, &host_buf);

    VkMemoryRequirements hmr;
    vkGetBufferMemoryRequirements(d.dev, host_buf, &hmr);
    auto hai = VkMemoryAllocateInfo{};
    hai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    hai.allocationSize = hmr.size;
    hai.memoryTypeIndex = find_type(d, hmr.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(d.dev, &hai, nullptr, &host_mem);
    vkBindBufferMemory(d.dev, host_buf, host_mem, 0);
    vkMapMemory(d.dev, host_mem, 0, VK_WHOLE_SIZE, 0, &mapped);

    LOG("gpu resources created (%dx%d) %d bytes",
        width, height, width * height * 4);
    return true;
}

void gpu_t::destroy(device_t &d) {
    vkUnmapMemory(d.dev, host_mem);
    vkDestroyBuffer(d.dev, host_buf, nullptr);
    vkFreeMemory(d.dev, host_mem, nullptr);
    vkDestroyImageView(d.dev, view, nullptr);
    vkDestroyImage(d.dev, img, nullptr);
    vkFreeMemory(d.dev, img_mem, nullptr);
}

}