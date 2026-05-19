import struct, sys
for name in ["quad_vert", "quad_frag"]:
    with open(f"src/vk/{name}.spv", "rb") as f:
        data = f.read()
    words = struct.iter_unpack("<I", data)
    hexes = ", ".join(f"0x{w[0]:08X}" for w in words)
    code = f"#pragma once\n#include <vulkan/vulkan.h>\nnamespace vk {{\nconstexpr unsigned char {name}_spv[] = {{{hexes}}};\nconstexpr size_t {name}_spv_size = {len(data)};\n}}\n"
    with open(f"src/vk/{name}_spv.h", "w") as f:
        f.write(code)
    print(f"wrote {name}_spv.h ({len(data)} bytes)")
