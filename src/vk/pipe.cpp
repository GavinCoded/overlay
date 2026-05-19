#include "vk/pipe.h"
#include "vk/quad_vert_spv.h"
#include "vk/quad_frag_spv.h"
#include "log.h"

namespace vk {

static VkShaderModule make_mod(device_t &d, const void *spv, long sz) {
    auto ci = VkShaderModuleCreateInfo{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = (size_t)sz;
    ci.pCode = (const uint32_t *)spv;
    VkShaderModule m;
    if (vkCreateShaderModule(d.dev, &ci, nullptr, &m) != VK_SUCCESS) return nullptr;
    return m;
}

bool pipe_t::create(device_t &d, VkRenderPass rp, VkImageView font_view) {
    vert = make_mod(d, quad_vert_spv, quad_vert_spv_size);
    frag = make_mod(d, quad_frag_spv, quad_frag_spv_size);
    if (!vert || !frag) return false;

    auto ss = VkPipelineShaderStageCreateInfo{};
    ss.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss.stage = VK_SHADER_STAGE_VERTEX_BIT; ss.module = vert; ss.pName = "main";
    auto ss2 = VkPipelineShaderStageCreateInfo{};
    ss2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss2.stage = VK_SHADER_STAGE_FRAGMENT_BIT; ss2.module = frag; ss2.pName = "main";
    VkPipelineShaderStageCreateInfo stages[] = {ss, ss2};

    auto bd = VkVertexInputBindingDescription{};
    bd.binding = 0; bd.stride = 32; bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    auto ad = VkVertexInputAttributeDescription{};
    ad.location = 0; ad.binding = 0; ad.format = VK_FORMAT_R32G32_SFLOAT; ad.offset = 0;
    auto ad2 = VkVertexInputAttributeDescription{};
    ad2.location = 1; ad2.binding = 0; ad2.format = VK_FORMAT_R32G32_SFLOAT; ad2.offset = 8;
    auto ad3 = VkVertexInputAttributeDescription{};
    ad3.location = 2; ad3.binding = 0; ad3.format = VK_FORMAT_R32G32B32A32_SFLOAT; ad3.offset = 16;
    VkVertexInputAttributeDescription ads[] = {ad, ad2, ad3};
    auto vii = VkPipelineVertexInputStateCreateInfo{};
    vii.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount = 1; vii.pVertexBindingDescriptions = &bd;
    vii.vertexAttributeDescriptionCount = 3; vii.pVertexAttributeDescriptions = ads;

    auto ia = VkPipelineInputAssemblyStateCreateInfo{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    auto vp = VkPipelineViewportStateCreateInfo{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1; vp.scissorCount = 1;

    VkDynamicState dyn_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    auto dyn_state = VkPipelineDynamicStateCreateInfo{};
    dyn_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state.dynamicStateCount = 2; dyn_state.pDynamicStates = dyn_states;

    auto rs = VkPipelineRasterizationStateCreateInfo{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE; rs.lineWidth = 1.0f;

    auto ms = VkPipelineMultisampleStateCreateInfo{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    auto cb = VkPipelineColorBlendAttachmentState{};
    cb.blendEnable = VK_TRUE;
    cb.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cb.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb.colorBlendOp = VK_BLEND_OP_ADD;
    cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb.alphaBlendOp = VK_BLEND_OP_ADD;
    cb.colorWriteMask = 0xF;
    auto cbs = VkPipelineColorBlendStateCreateInfo{};
    cbs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs.attachmentCount = 1; cbs.pAttachments = &cb;

    auto dslb = VkDescriptorSetLayoutBinding{};
    dslb.binding = 0; dslb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dslb.descriptorCount = 1; dslb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    auto dsci = VkDescriptorSetLayoutCreateInfo{};
    dsci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsci.bindingCount = 1; dsci.pBindings = &dslb;
    vkCreateDescriptorSetLayout(d.dev, &dsci, nullptr, &dsl);

    auto pci = VkPushConstantRange{};
    pci.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; pci.size = 64; pci.offset = 0;
    auto plci = VkPipelineLayoutCreateInfo{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount = 1; plci.pPushConstantRanges = &pci;
    plci.setLayoutCount = 1; plci.pSetLayouts = &dsl;
    vkCreatePipelineLayout(d.dev, &plci, nullptr, &layout);

    auto sci = VkSamplerCreateInfo{};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter = VK_FILTER_NEAREST; sci.minFilter = VK_FILTER_NEAREST;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(d.dev, &sci, nullptr, &sampler);

    auto dpi = VkDescriptorPoolSize{};
    dpi.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; dpi.descriptorCount = 1;
    auto dpci = VkDescriptorPoolCreateInfo{};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets = 1; dpci.poolSizeCount = 1; dpci.pPoolSizes = &dpi;
    vkCreateDescriptorPool(d.dev, &dpci, nullptr, &dp);

    auto dai = VkDescriptorSetAllocateInfo{};
    dai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dai.descriptorPool = dp; dai.descriptorSetCount = 1; dai.pSetLayouts = &dsl;
    vkAllocateDescriptorSets(d.dev, &dai, &ds);

    auto di = VkDescriptorImageInfo{};
    di.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    di.imageView = font_view; di.sampler = sampler;
    auto dw = VkWriteDescriptorSet{};
    dw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dw.dstSet = ds; dw.dstBinding = 0; dw.descriptorCount = 1;
    dw.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dw.pImageInfo = &di;
    vkUpdateDescriptorSets(d.dev, 1, &dw, 0, nullptr);

    auto gci = VkGraphicsPipelineCreateInfo{};
    gci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gci.stageCount = 2; gci.pStages = stages;
    gci.pVertexInputState = &vii; gci.pInputAssemblyState = &ia;
    gci.pViewportState = &vp; gci.pRasterizationState = &rs;
    gci.pMultisampleState = &ms; gci.pColorBlendState = &cbs;
    gci.pDynamicState = &dyn_state;
    gci.layout = layout; gci.renderPass = rp;
    if (vkCreateGraphicsPipelines(d.dev, VK_NULL_HANDLE, 1, &gci, nullptr, &pipeline) != VK_SUCCESS)
        { LOGS("pipeline failed"); return false; }
    LOGS("pipeline created");
    return true;
}

void pipe_t::destroy(device_t &d) {
    vkDestroyDescriptorPool(d.dev, dp, nullptr);
    vkDestroyDescriptorSetLayout(d.dev, dsl, nullptr);
    vkDestroySampler(d.dev, sampler, nullptr);
    vkDestroyPipeline(d.dev, pipeline, nullptr);
    vkDestroyPipelineLayout(d.dev, layout, nullptr);
    vkDestroyShaderModule(d.dev, frag, nullptr);
    vkDestroyShaderModule(d.dev, vert, nullptr);
}

}