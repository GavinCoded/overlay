#pragma once
#include <cstdint>

struct rgba_t { float r, g, b, a; };

inline rgba_t rgba_premul(float r, float g, float b, float a) {
    return {r * a, g * a, b * a, a};
}