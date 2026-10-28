#version 460
// Public-domain P4.3 fixture; both attributes come directly from compute buffers.
layout(location=0) in vec4 position;
layout(location=1) in vec2 uv;
layout(location=0) out vec2 texture_uv;
void main() { gl_Position=position; texture_uv=uv; }
