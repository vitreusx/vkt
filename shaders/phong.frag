#version 450

layout(set = 3, binding = 0) uniform sampler2D textures[64];

layout(std140, set = 0, binding = 1) uniform Scene {
  vec3 ambientLight;
  vec3 lightPos;
  vec3 lightColor;
  vec3 cameraPos;
  int shadingModel;
};

#define BLEND_OP_MUL 0
#define BLEND_OP_ADD 1

struct TexStack {
  vec3 color;
  int texIndex;
  int blendOp;
  float blendFactor;
};

layout(std140, set = 2, binding = 0) uniform Material {
  TexStack ambient;
  TexStack diffuse;
  TexStack specular;
  float shininess;
};

vec3 evalStack(in TexStack stack, in vec2 uv) {
  vec3 outColor = stack.color;
  if (stack.texIndex > 0) {
    vec3 texColor =
        stack.blendFactor * vec3(texture(textures[stack.texIndex], uv));
    if (stack.blendOp == BLEND_OP_MUL) {
      outColor = outColor * texColor;
    } else if (stack.blendOp == BLEND_OP_ADD) {
      outColor = outColor + texColor;
    }
  }
  return outColor;
}

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 ambient = evalStack(ambient, fragTexCoord);
  ambient = ambient * ambientLight;

  vec3 diffuse = evalStack(diffuse, fragTexCoord);
  vec3 norm = normalize(fragNormal);
  vec3 lightDir = normalize(lightPos - fragPos);
  float diffuseCoef = max(dot(norm, lightDir), 0.0);
  diffuse = diffuse * diffuseCoef * lightColor;

  vec3 specular = evalStack(specular, fragTexCoord);
  vec3 viewDir = normalize(cameraPos - fragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float specularCoef = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
  specular = specular * specularCoef * lightColor;

  vec3 finalColor = (ambient + diffuse + specular) * fragColor;
  outColor = vec4(finalColor, 1.0);
}