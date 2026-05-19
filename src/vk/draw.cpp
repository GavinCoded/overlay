#include "vk/draw.h"
#include "vk/util.h"
#include "log.h"
#include <cstring>

namespace vk {

struct vertex { float x, y, u, v, r, g, b, a; };

bool draw_t::create(device_t &d, int width, int height) {
    w = width; h = height; quad_count = 0; cur_pipe = nullptr;
    auto bci = VkBufferCreateInfo{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = 65536;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(d.dev, &bci, nullptr, &vb);

    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(d.dev, vb, &mr);
    auto ai = VkMemoryAllocateInfo{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = mr.size;
    ai.memoryTypeIndex = find_type(d, mr.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(d.dev, &ai, nullptr, &vb_mem);
    vkBindBufferMemory(d.dev, vb, vb_mem, 0);
    vkMapMemory(d.dev, vb_mem, 0, VK_WHOLE_SIZE, 0, &vb_map);
    LOG("draw buffer: %d bytes, %d max quads", 65536, 65536 / 6 / 32);
    return true;
}

void draw_t::destroy(device_t &d) {
    vkUnmapMemory(d.dev, vb_mem);
    vkDestroyBuffer(d.dev, vb, nullptr);
    vkFreeMemory(d.dev, vb_mem, nullptr);
}
// test
void draw_t::begin(device_t &d, rp_t &rp, pipe_t &pipe) {
    quad_count = 0; cur_pipe = &pipe;
    auto aci = VkCommandBufferAllocateInfo{};
    aci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aci.commandPool = d.pool;
    aci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aci.commandBufferCount = 1;
    vkAllocateCommandBuffers(d.dev, &aci, &cmd);

    auto bbi = VkCommandBufferBeginInfo{};
    bbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &bbi);

    auto clear = VkClearValue{};
    clear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    auto rpbi = VkRenderPassBeginInfo{};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = rp.rp;
    rpbi.framebuffer = rp.fb;
    rpbi.renderArea = {{0, 0}, {(uint32_t)w, (uint32_t)h}};
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clear;
    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp = {0, 0, (float)w, (float)h, 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    VkRect2D sc = {{0, 0}, {(uint32_t)w, (uint32_t)h}};
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.pipeline);
}

void draw_t::add_quad(float x, float y, float gw, float gh,
    float u, float v, float uw, float vh,
    float r, float g, float bl, float a)
{
    if (quad_count >= 341) return;
    auto verts = (vertex *)vb_map + quad_count * 6;
    vertex v0 = {x, y, u, v, r, g, bl, a};
    vertex v1 = {x + gw, y, u + uw, v, r, g, bl, a};
    vertex v2 = {x + gw, y + gh, u + uw, v + vh, r, g, bl, a};
    vertex v3 = {x, y, u, v, r, g, bl, a};
    vertex v4 = {x + gw, y + gh, u + uw, v + vh, r, g, bl, a};
    vertex v5 = {x, y + gh, u, v + vh, r, g, bl, a};
    verts[0] = v0; verts[1] = v1; verts[2] = v2;
    verts[3] = v3; verts[4] = v4; verts[5] = v5;
    quad_count++;
}

void draw_t::end(device_t &d, rp_t &rp, gpu_t &gpu) {
    if (quad_count > 0) {
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &off);
        float pc[16] = {};
        pc[0] = 2.0f / w; pc[5] = 2.0f / h;
        pc[10] = 1.0f; pc[12] = -1.0f; pc[13] = -1.0f; pc[15] = 1.0f;
        vkCmdPushConstants(cmd, cur_pipe->layout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, 64, pc);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            cur_pipe->layout, 0, 1, &cur_pipe->ds, 0, nullptr);
        vkCmdDraw(cmd, quad_count * 6, 1, 0, 0);
    }
    vkCmdEndRenderPass(cmd);

    auto bar = VkImageMemoryBarrier{};
    bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar.image = gpu.img;
    bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    bar.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    bar.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar);

    auto region = VkBufferImageCopy{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {(uint32_t)w, (uint32_t)h, 1};
    vkCmdCopyImageToBuffer(cmd, gpu.img,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, gpu.host_buf, 1, &region);

    vkEndCommandBuffer(cmd);

    auto si = VkSubmitInfo{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(d.queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(d.queue);

    vkFreeCommandBuffers(d.dev, d.pool, 1, &cmd);
}

void draw_t::copy_to(const void *src, void *dib, int dib_pitch) {
    auto s = (const uint8_t *)src;
    auto d = (uint8_t *)dib;
    for (int y = 0; y < h; y++)
        memcpy(d + y * dib_pitch, s + y * w * 4, (size_t)w * 4);
}

}