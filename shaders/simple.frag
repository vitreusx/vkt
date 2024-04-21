#version 450

#define MAX_TEXTURES 4096

#define RENDER_SET 0
#define MATERIAL_SET 1

layout(set = RENDER_SET, binding = 1)
uniform sampler2D textures[MAX_TEXTURES];

layout(set = MATERIAL_SET, binding = 0)
uniform MaterialData {
  int textureIdx;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
  outColor = texture(textures[textureIdx], fragTexCoord);
}