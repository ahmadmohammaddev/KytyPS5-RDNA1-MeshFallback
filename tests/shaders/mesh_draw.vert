#version 460
// Public-domain authored P4.2 fixture; position comes from compute's output buffer.
layout(location = 0) in vec4 position;
void main() { gl_Position = position; }
