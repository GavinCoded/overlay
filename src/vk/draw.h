#pragma once
#include "vk/types.h"
#include "vk/device.h"
#include "vk/pipe.h"
#include "vk/gpu.h"
#include "vk/rp.h"
#include <string>

namespace vk {

struct draw_t {
    VkBuffer vb;
    VkDeviceMemory vb_mem;
    void *vb_map;
    VkCommandBuffer cmd;
    int quad_count;
    int w, h;
    pipe_t *cur_pipe;

    bool create(device_t &d, int width, int height);
    void destroy(device_t &d);
    void begin(device_t &d, rp_t &rp, pipe_t &pipe);
    void add_quad(float x, float y, float gw, float gh,
        float u, float v, float uw, float vh,
        float r, float g, float bl, float a);
    void end(device_t &d, rp_t &rp, gpu_t &gpu);
    void copy_to(const void *src, void *dib, int dib_pitch);
};

}