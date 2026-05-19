#include "vk/font.h"
#include "vk/font_data.h"
#include "vk/util.h"
#include "log.h"

namespace vk {

bool font_t::create(device_t &d) {
    auto ci = VkImageCreateInfo{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = VK_FORMAT_R8_UNORM;
    ci.extent = {(uint32_t)font_atlas_w, (uint32_t)font_atlas_h, 1};
    ci.mipLevels = 1; ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
    vkAllocateMemory(d.dev, &ai, nullptr, &mem);
    vkBindImageMemory(d.dev, img, mem, 0);

    auto ivci = VkImageViewCreateInfo{};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = img;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R8_UNORM;
    ivci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(d.dev, &ivci, nullptr, &view);

    VkBuffer stage;
    VkDeviceMemory stage_mem;
    auto bci = VkBufferCreateInfo{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    auto pix = font_atlas_w * font_atlas_h;
    bci.size = pix;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkCreateBuffer(d.dev, &bci, nullptr, &stage);

    VkMemoryRequirements smr;
    vkGetBufferMemoryRequirements(d.dev, stage, &smr);
    auto sai = VkMemoryAllocateInfo{};
    sai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    sai.allocationSize = smr.size;
    sai.memoryTypeIndex = find_type(d, smr.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(d.dev, &sai, nullptr, &stage_mem);
    vkBindBufferMemory(d.dev, stage, stage_mem, 0);

    void *ptr;
    vkMapMemory(d.dev, stage_mem, 0, VK_WHOLE_SIZE, 0, &ptr);
    auto dst = (uint8_t *)ptr;
    for (int c = 0; c < font_chars; c++) {
        int cx = (c % font_cols) * font_width;
        int cy = (c / font_cols) * font_height;
        for (int y = 0; y < font_height; y++) {
            auto row = font_data[c * font_height + y];
            for (int x = 0; x < font_width; x++) {
                auto px = (row >> (7 - x)) & 1;
                dst[(cy + y) * font_atlas_w + cx + x] = px ? 255 : 0;
            }
        }
    }
    vkUnmapMemory(d.dev, stage_mem);

    auto cmd = VkCommandBuffer{};
    auto aci = VkCommandBufferAllocateInfo{};
    aci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aci.commandPool = d.pool;
    aci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aci.commandBufferCount = 1;
    vkAllocateCommandBuffers(d.dev, &aci, &cmd);

    auto bbi = VkCommandBufferBeginInfo{};
    bbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &bbi);

    auto bar1 = VkImageMemoryBarrier{};
    bar1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar1.image = img;
    bar1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bar1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar1.srcAccessMask = 0;
    bar1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bar1.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar1);

    auto region = VkBufferImageCopy{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {(uint32_t)font_atlas_w, (uint32_t)font_atlas_h, 1};
    vkCmdCopyBufferToImage(cmd, stage, img,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    auto bar2 = VkImageMemoryBarrier{};
    bar2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar2.image = img;
    bar2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bar2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bar2.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar2);

    vkEndCommandBuffer(cmd);
    auto si = VkSubmitInfo{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(d.queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(d.queue);

    vkFreeCommandBuffers(d.dev, d.pool, 1, &cmd);
    vkDestroyBuffer(d.dev, stage, nullptr);
    vkFreeMemory(d.dev, stage_mem, nullptr);

    LOG("font texture created (%dx%d)", font_atlas_w, font_atlas_h);
    return true;
}

void font_t::destroy(device_t &d) {
    vkDestroyImageView(d.dev, view, nullptr);
    vkDestroyImage(d.dev, img, nullptr);
    vkFreeMemory(d.dev, mem, nullptr);
}

}