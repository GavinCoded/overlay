#version 450
layout(location = 0) in vec2 frag_uv;
layout(location = 1) in vec4 frag_color;
layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform sampler2D font_tex;
void main() {
    float a = texture(font_tex, frag_uv).r;
    out_color = vec4(frag_color.rgb, frag_color.a * a);
}