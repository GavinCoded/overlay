#include "vk/rp.h"
#include "vk/util.h"
#include "log.h"

namespace vk {

bool rp_t::create(device_t &d, VkImageView iv, int w, int h) {
    auto ad = VkAttachmentDescription{};
    ad.format = VK_FORMAT_B8G8R8A8_UNORM;
    ad.samples = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ad.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    auto ar = VkAttachmentReference{};
    ar.attachment = 0;
    ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    auto sd = VkSubpassDescription{};
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.colorAttachmentCount = 1;
    sd.pColorAttachments = &ar;

    auto rci = VkRenderPassCreateInfo{};
    rci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rci.attachmentCount = 1;
    rci.pAttachments = &ad;
    rci.subpassCount = 1;
    rci.pSubpasses = &sd;
    if (vkCreateRenderPass(d.dev, &rci, nullptr, &rp) != VK_SUCCESS)
        return false;

    auto fci = VkFramebufferCreateInfo{};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = rp;
    fci.attachmentCount = 1;
    fci.pAttachments = &iv;
    fci.width = (uint32_t)w;
    fci.height = (uint32_t)h;
    fci.layers = 1;
    vkCreateFramebuffer(d.dev, &fci, nullptr, &fb);
    LOG("render pass + framebuffer created");
    return true;
}

void rp_t::destroy(device_t &d) {
    vkDestroyFramebuffer(d.dev, fb, nullptr);
    vkDestroyRenderPass(d.dev, rp, nullptr);
}

}
