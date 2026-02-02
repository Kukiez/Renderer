#pragma once
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Resource/Buffer/BufferKey.h>
#include <Renderer/Resource/Geometry/GeometryComponentType.h>
#include <Renderer/Resource/Geometry/GeometryKey.h>
#include <Renderer/Resource/Texture/TextureKey.h>

#include "ModelCooking.h"
#include "ModelHierarchyAsset.h"

class ModelKey : public UnsignedKeyBase {
public:
    using UnsignedKeyBase::UnsignedKeyBase;
};

class TextureResourceType;
class ModelResourceSystem;
class BufferResourceStorage;
class TextureComponentType;

struct GPUMeshSection {
    unsigned baseVertex{};
    unsigned indexOffset{};
    unsigned indexCount{};
    unsigned materialIndex{};
};

struct GPUPBRMaterial {
    glm::vec4 albedoFactor{};

    float metallicFactor{};
    float roughnessFactor{};

    unsigned diffuseTex{};
    unsigned normalTex{};

    unsigned metallicRoughnessTex{};
    unsigned occlusionTex{};
    unsigned emissiveTex{};

    unsigned pad0{};
};

struct ModelRuntimeDrawData {
    GeometryKey geometry{};

    TextureKey* textures{};
    size_t numTextures{};

    size_t gpuMeshSectionIndex{};
    size_t gpuMaterialIndex{};

    GPUMeshSection* meshSections{};
    size_t numMeshSections{};

    ModelCooking::MeshIndices* meshIndices{};

    mem::range<const GPUMeshSection> getMeshSections() const {
        return mem::make_range(meshSections, numMeshSections);
    }
};

struct ModelData {
    ModelHierarchyAsset modelHierarchy;
    ModelRuntimeDrawData drawData;
};

class ModelAsset;

class ModelResourceSystem {
    BufferKey meshSectionsBuffer{};
    BufferKey pbrMaterialsBuffer{};

    size_t nextFreeSectionIdx = 0;
    size_t nextFreePBRMaterialIdx = 0;

    size_t meshSectionsCapacity = 64;
    size_t pbrMaterialsCapacity = 64;

    TextureKey defaultAlbedo;
    TextureKey defaultEmissive;
    TextureKey defaultNormal;
    TextureKey defaultMetallicRoughness;
    TextureKey defaultOcclusion;

    std::vector<ModelData> assets;

    BufferResourceStorage* buffers{};
    TextureResourceType* textureResource{};
    GeometryResourceStorage* geometryResource{};
public:
    ModelResourceSystem() = default;
    ModelResourceSystem(BufferResourceStorage* buffers, TextureResourceType* textures, GeometryResourceStorage* geometry) : buffers(buffers), textureResource(textures), geometryResource(geometry) {}

    ModelResourceSystem(const ModelResourceSystem&) = delete;
    ModelResourceSystem& operator = (const ModelResourceSystem&) = delete;

    ModelResourceSystem(ModelResourceSystem&&) = default;
    ModelResourceSystem& operator = (ModelResourceSystem&&) = default;

    void onLoad(RendererLoadView view);

    void reallocateMeshSectionBuffer(size_t newMeshSectionsCapacity);

    void reallocatePBRMaterialBuffer(size_t newPBRMaterialCapacity);

    ModelKey loadModel(const ModelCooking::ModelLoadingCache& cache);

    ModelAsset getAsset(ModelKey key);

    const ModelData* getModelData(const ModelKey key) const {
        return &assets[key.index()];
    }

    BufferKey getMeshSectionsBuffer() const { return meshSectionsBuffer; }
    BufferKey getPBRMaterialsBuffer() const { return pbrMaterialsBuffer; }
};

class ModelAsset {
    ModelResourceSystem* modelType{};
    ModelKey key{};
    const ModelData* modelData{};
public:
    ModelAsset() = default;
    ModelAsset(ModelResourceSystem* models, const ModelKey key) : modelType(models), key(key), modelData(models->getModelData(key)) {}

    ModelKey getKey() const { return key; }

    GeometryKey getGeometry() const {
        return modelType->getModelData(key)->drawData.geometry;
    }

    TextureKey getTexture(size_t index) const {
        assert(index < modelData->drawData.numTextures);
        return modelData->drawData.textures[index];
    }

    mem::range<const GPUMeshSection> getMeshSections() const {
        return modelData->drawData.getMeshSections();
    }

    mem::range<const ModelCooking::MeshIndices> getMeshIndices() const {
        return mem::make_range(modelData->drawData.meshIndices, modelData->modelHierarchy.numNodes);
    }

    mem::range<const Transform> getLocalTransforms() const {
        return mem::make_range(modelData->modelHierarchy.localNodeTransforms, modelData->modelHierarchy.numNodes);
    }

    const ModelData* getModelData() const { return modelData; }

    const ModelHierarchyAsset& getHierarchy() const { return modelData->modelHierarchy; }
};

inline ModelAsset ModelResourceSystem::getAsset(const ModelKey key) {
    return ModelAsset(this, key);
}