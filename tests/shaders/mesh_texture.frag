#version 460
// Public-domain P4.3 fixture. Smooth interpolation (w=1 here), nearest sampling.
layout(location=0) in vec2 texture_uv;
layout(set=0,binding=0) uniform sampler2D authored_texture;
layout(location=0) out vec4 color;
void main() { color=texture(authored_texture,texture_uv); }
