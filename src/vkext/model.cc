#include <vkext/model.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

static std::filesystem::path toPath(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  return path;
}

namespace vkext {
Model::Model(std::filesystem::path const &path) {
  Assimp::Importer importer;

  // Remove lines and points from the mesh
  importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                              aiPrimitiveType_LINE | aiPrimitiveType_POINT);

  auto scene = importer.ReadFile(
      path.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                         aiProcess_SortByPType | aiProcess_TransformUVCoords |
                         aiProcess_GenUVCoords | aiProcess_SplitLargeMeshes);

  for (uint32_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx) {
    auto *aiMesh = scene->mMeshes[meshIdx];
    auto &mesh = meshes.emplace_back();

    mesh.pos.resize(aiMesh->mNumVertices);
    for (uint32_t vertIdx = 0; vertIdx < aiMesh->mNumVertices; ++vertIdx) {
      auto aiVert = aiMesh->mVertices[vertIdx];
      mesh.pos[vertIdx] = {aiVert.x, aiVert.y, aiVert.z};
    }

    int numUV = aiMesh->GetNumUVChannels();
    mesh.texCoord.resize(numUV);
    for (int uvIdx = 0; uvIdx < numUV; ++uvIdx) {
      auto &texUV = mesh.texCoord[uvIdx];
      texUV.resize(aiMesh->mNumVertices);
      for (uint32_t vertIdx = 0; vertIdx < aiMesh->mNumVertices; ++vertIdx) {
        auto aiTexUV = aiMesh->mTextureCoords[uvIdx][vertIdx];
        texUV[vertIdx] = {aiTexUV.x, aiTexUV.y};
      }
    }

    if (aiMesh->HasVertexColors(0)) {
      mesh.color.resize(aiMesh->mNumVertices);
      for (uint32_t vertIdx = 0; vertIdx < aiMesh->mNumVertices; ++vertIdx) {
        auto aiColor = aiMesh->mColors[0][vertIdx];
        mesh.color[vertIdx] = {aiColor.r, aiColor.g, aiColor.b};
      }
    }

    if (aiMesh->HasNormals()) {
      mesh.normal.resize(aiMesh->mNumVertices);
      for (uint32_t vertIdx = 0; vertIdx < aiMesh->mNumVertices; ++vertIdx) {
        auto aiNormal = aiMesh->mNormals[vertIdx];
        mesh.normal[vertIdx] = {aiNormal.x, aiNormal.y, aiNormal.z};
      }
    }

    for (uint32_t faceIdx = 0; faceIdx < aiMesh->mNumFaces; ++faceIdx)
      for (uint32_t indIdx = 0; indIdx < 3; ++indIdx)
        mesh.indices.push_back(aiMesh->mFaces[faceIdx].mIndices[indIdx]);
  }

  for (uint32_t matIdx = 0; matIdx < scene->mNumMaterials; ++matIdx) {
    auto *aiMat = scene->mMaterials[matIdx];
    auto &mat = materials.emplace_back();

#define LOAD_TEX(name, type)                                                   \
  {                                                                            \
    aiColor4D color;                                                           \
    if (aiMat->Get("$clr." #name, 0, 0, color) == AI_SUCCESS)                  \
      mat.name.baseColor = {color.r, color.g, color.b, color.a};               \
                                                                               \
    int numTextures = aiMat->GetTextureCount(type);                            \
    for (int texIdx = 0; texIdx < numTextures; ++texIdx) {                     \
      auto &layer = mat.name.layers.emplace_back();                            \
      aiMat->Get(AI_MATKEY_TEXOP(type, texIdx), layer.op);                     \
      aiMat->Get(AI_MATKEY_TEXBLEND(type, texIdx), layer.op);                  \
      aiString aiTexName;                                                      \
      aiMat->Get(AI_MATKEY_TEXTURE(type, texIdx), aiTexName);                  \
      auto texName = std::string(aiTexName.data, aiTexName.length);            \
      layer.texPath = path.parent_path() / toPath(texName);                    \
      aiMat->Get(AI_MATKEY_MAPPINGMODE_U(type, texIdx), layer.mapModeU);       \
      aiMat->Get(AI_MATKEY_MAPPINGMODE_V(type, texIdx), layer.mapModeV);       \
      aiMat->Get(AI_MATKEY_UVWSRC(type, texIdx), layer.uvChannel);             \
    }                                                                          \
  };                                                                           \
  (void)0

    LOAD_TEX(ambient, aiTextureType_AMBIENT);
    LOAD_TEX(diffuse, aiTextureType_DIFFUSE);
    LOAD_TEX(specular, aiTextureType_SPECULAR);
    LOAD_TEX(transmission, aiTextureType_TRANSMISSION);
    LOAD_TEX(emission, aiTextureType_EMISSIVE);

#undef LOAD_TEX

    aiMat->Get(AI_MATKEY_SHININESS, mat.shininess);
    aiMat->Get(AI_MATKEY_REFRACTI, mat.ior);
    aiMat->Get(AI_MATKEY_OPACITY, mat.opacity);
    aiMat->Get(AI_MATKEY_SHADING_MODEL, mat.shadingModel);
  }
}
} // namespace vkext