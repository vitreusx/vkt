#version 450

layout(std140, set = 0, binding = 0) uniform VP {
  mat4 view;
  mat4 proj;
};

layout(std140, set = 1, binding = 0) uniform Model {
  mat4 model;
  mat3 normalMtx;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

void main() {
  gl_Position = proj * view * model * vec4(inPosition, 1.0);
  fragPos = vec3(model * vec4(inPosition, 1.0));
  fragColor = inColor;
  fragNormal = normalMtx * inNormal;
  fragTexCoord = inTexCoord;
}