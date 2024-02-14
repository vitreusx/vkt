#version 450

layout(set = 1, binding = 0) uniform MaterialData {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 transmittance;
  vec3 emission;
  float shininess;
  float ior;
  float dissolve;
  int illum;
}
mat;

layout(set = 1, binding = 1) uniform sampler2D ambientTex;
layout(set = 1, binding = 2) uniform sampler2D diffuseTex;
layout(set = 1, binding = 3) uniform sampler2D specularTex;
layout(set = 1, binding = 4) uniform sampler2D specularHighlightTex;
layout(set = 1, binding = 5) uniform sampler2D bumpTex;
layout(set = 1, binding = 6) uniform sampler2D displacementTex;
layout(set = 1, binding = 7) uniform sampler2D alphaTex;
layout(set = 1, binding = 8) uniform sampler2D reflectionTex;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
  outColor = texture(diffuseTex, fragTexCoord);
}