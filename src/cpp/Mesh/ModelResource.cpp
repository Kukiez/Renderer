#include "Mesh/ModelResource.h"

#include <Renderer/Resource/Buffer/BufferComponentType.h>
#include <Renderer/Resource/Geometry/GeometryBuilder.h>
#include <Renderer/Resource/Geometry/VertexLayout.h>
#include <Renderer/Resource/Texture/TextureFactory.h>

void ModelResourceSystem::onLoad(RendererLoadView view) {
    this->geometryResource = &view.getRenderer().getGeometryStorage();
    this->textureResource = &view.getRenderer().getTextureStorage();
    reallocateMeshSectionBuffer(meshSectionsCapacity);
    reallocatePBRMaterialBuffer(pbrMaterialsCapacity);
}

void ModelResourceSystem::reallocateMeshSectionBuffer(size_t newMeshSectionsCapacity) {
    BufferKey prevMeshSections = meshSectionsBuffer;

    meshSectionsBuffer = buffers->createBuffer<GPUMeshSection>(newMeshSectionsCapacity, BufferUsageHint::IMMUTABLE);

    if (prevMeshSections) {
        buffers->copyBufferRange({
            .src = prevMeshSections,
            .dst = meshSectionsBuffer,
            .srcOffset = 0,
            .dstOffset = 0,
            .sizeInBytes = nextFreeSectionIdx * sizeof(GPUMeshSection)
        });
        buffers->destroyBuffer(prevMeshSections);
    }
    meshSectionsCapacity = newMeshSectionsCapacity;
}

void ModelResourceSystem::reallocatePBRMaterialBuffer(size_t newPBRMaterialCapacity) {
    BufferKey prevPBRMaterials = pbrMaterialsBuffer;

    pbrMaterialsBuffer = buffers->createBuffer<GPUPBRMaterial>(newPBRMaterialCapacity, BufferUsageHint::IMMUTABLE);

    if (prevPBRMaterials) {
        buffers->copyBufferRange({
            .src = prevPBRMaterials,
            .dst = pbrMaterialsBuffer,
            .srcOffset = 0,
            .dstOffset = 0,
            .sizeInBytes = nextFreePBRMaterialIdx * sizeof(GPUPBRMaterial)
        });
        buffers->destroyBuffer(prevPBRMaterials);
    }
    pbrMaterialsCapacity = newPBRMaterialCapacity;
}

ModelKey ModelResourceSystem::loadModel(const ModelCooking::ModelLoadingCache& cache) {
    TextureFactory tsf(textureResource);

    auto* textures = new TextureKey[cache.textures.size()];

    size_t i = 0;
    for (auto& texture : cache.textures) {
        TextureKey key;

        switch (texture.type) {
            case ModelCooking::TextureType::DIFFUSE:
            case ModelCooking::TextureType::NORMAL: {
                TextureCreateParams params;
                params.format = TextureFormat::RGB_8;
                params.minFilter = TextureMinFilter::NEAREST;
                params.magFilter = TextureMagFilter::NEAREST;
                params.wrap = TextureWrap::REPEAT;

                key = tsf.createTexture2D(TextureDescriptor2D(
                    params,
                    ImageLoader::load(texture.path, ImageLoadOptions::NONE)
                    )
                );
                break;
            }

            case ModelCooking::TextureType::GLTF_METALLIC_ROUGHNESS: {
                TextureCreateParams metallicRoughnessParams;
                metallicRoughnessParams.format = TextureFormat::RGB_8;
                metallicRoughnessParams.minFilter = TextureMinFilter::NEAREST;
                metallicRoughnessParams.magFilter = TextureMagFilter::NEAREST;
                metallicRoughnessParams.wrap = TextureWrap::REPEAT;
                key = tsf.createTexture2D(TextureDescriptor2D(metallicRoughnessParams, ImageLoader::load(texture.path, ImageLoadOptions::NONE)));
                break;
            }
            default: assert(false);
        }
        textures[i] = key;
        ++i;
    }

    static constexpr auto ModelVertexLayout = VertexLayoutOf(
        VertexAttribute::Position3, VertexAttribute::Normal3,
        VertexAttribute::TexCoord2, VertexAttrib::Tangent3,
        VertexAttrib::Bitangent3,
        VertexAttribute::BoneIds4,
        VertexAttribute::BoneWeights4
    );

    GeometryFactory geoF(*geometryResource);

    GeometryKey geometry = geoF.loadGeometry(GeometryDescriptor(
        mem::make_range(cache.vertices, cache.numVertices),
        mem::make_range(cache.indices, cache.numIndices),
        ModelVertexLayout
    ));

    ModelRuntimeDrawData renderData;
    renderData.geometry = geometry;
    renderData.textures = textures;
    renderData.gpuMeshSectionIndex = nextFreeSectionIdx;
    renderData.gpuMaterialIndex = nextFreePBRMaterialIdx;

    renderData.meshIndices = new ModelCooking::MeshIndices[cache.numNodes];

    unsigned* meshIndicesPool = new unsigned[cache.getTotalMeshIndices()];

    unsigned nextMeshIdx = 0;
    for (unsigned m = 0; m < cache.numNodes; ++m) {
        auto& meshIndices = cache.nodeToMeshIndices[m];

        renderData.meshIndices[m].indices = meshIndicesPool + nextMeshIdx;
        renderData.meshIndices[m].numIndices = meshIndices.numIndices;

        std::memcpy(meshIndicesPool + nextMeshIdx, meshIndices.indices, meshIndices.numIndices * sizeof(unsigned));
        nextMeshIdx += meshIndices.numIndices;
    }
    renderData.meshSections = new GPUMeshSection[cache.numMeshes];

    auto meshSections = meshSectionsBuffer.mapRange<GPUMeshSection>(nextFreeSectionIdx, cache.numMeshes);
    auto pbrMaterials = pbrMaterialsBuffer.mapRange<GPUPBRMaterial>(nextFreePBRMaterialIdx, cache.numMaterials);

    if (nextFreeSectionIdx + cache.numMeshes >= meshSectionsCapacity) {
        reallocateMeshSectionBuffer(meshSectionsCapacity * 2);
    }

    if (nextFreePBRMaterialIdx + cache.numMaterials >= pbrMaterialsCapacity) {
        reallocatePBRMaterialBuffer(pbrMaterialsCapacity * 2);
    }

    for (unsigned m = 0; m < cache.numMeshes; ++m) {
        auto& mesh = cache.meshes[m];

        GPUMeshSection section;
        section.indexOffset = mesh.indexOffset;
        section.indexCount = mesh.numIndices;
        section.materialIndex = renderData.gpuMaterialIndex + mesh.materialIndex;
        section.baseVertex = mesh.vertexOffset;

        meshSections[m] = section;

        renderData.meshSections[m] = section;
    }

    for (unsigned m = 0; m < cache.numMaterials; ++m) {
        auto& material = cache.materials[m];

        GPUPBRMaterial gpuMaterial;
        gpuMaterial.albedoFactor = material.pbr.baseColorFactor;
        gpuMaterial.metallicFactor = material.pbr.metallicFactor;
        gpuMaterial.roughnessFactor = material.pbr.roughnessFactor;

        TextureKey albedo = material.pbr.albedoTexture != -1 ? textures[material.pbr.albedoTexture] : defaultAlbedo;
        TextureKey normal = material.pbr.normalTexture != -1 ? textures[material.pbr.normalTexture] : defaultNormal;
        TextureKey metallicRoughness = material.pbr.metallicRoughnessTexture != -1 ? textures[material.pbr.metallicRoughnessTexture] : defaultMetallicRoughness;
        TextureKey occlusion = material.pbr.occlusionTexture != -1 ? textures[material.pbr.occlusionTexture] : defaultOcclusion;
        TextureKey emissive = material.pbr.emissiveTexture != -1 ? textures[material.pbr.emissiveTexture] : defaultEmissive;

        gpuMaterial.diffuseTex = albedo.id();
        gpuMaterial.normalTex = normal.id();
        gpuMaterial.metallicRoughnessTex = metallicRoughness.id();
        gpuMaterial.occlusionTex = occlusion.id();
        gpuMaterial.emissiveTex = emissive.id();

        pbrMaterials[m] = gpuMaterial;
    }
    nextFreeSectionIdx += cache.numMeshes;
    nextFreePBRMaterialIdx += cache.numMaterials;

    assets.emplace_back(std::move(cache), renderData);
    return ModelKey{assets.size() - 1};
}
