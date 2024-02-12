#version 450

layout(binding = 0) uniform UniformValues {
    mat4 mvp;
} unif;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = unif.mvp * vec4(position, 1.0);
    // gl_Position = vec4(position, 0.0, 1.0);
    fragColor = color;
}