#pragma once
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <Math/Shapes/AABB.h>
#include "Animation/ModelAnimation.h"
#include <Renderer/Common/Transform.h>

struct aiString;
struct aiMesh;
struct aiNode;
struct aiScene;

class RigidModelData {

};

struct ModelCookingParams {
    std::string_view path{};
};

class ModelCooking {
public:
    struct FlattenedAssimpHierarchy {
        const aiNode* node{};
        int firstChild{};
        int numChildren{};
    };

    struct Vertex {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 uv0{};
        glm::vec3 tangent{};
        glm::vec3 bitangent{};
        glm::ivec4 boneIndices{};
        glm::vec4 boneWeights{};
    };

    struct Mesh {
        unsigned vertexOffset{};
        unsigned indexOffset{};
        unsigned numIndices{};
        unsigned materialIndex{};
    };

    struct Bone {
        glm::mat4 offsetMatrix{};
        int nodeID = -1;
    };

    enum class TextureType {
        DIFFUSE,
        NORMAL,
        GLTF_METALLIC_ROUGHNESS,
        EMISSIVE,
        DISPLACEMENT,
        HEIGHT,
        AMBIENT_OCCLUSION
    };

    struct Texture {
        std::string path{};
        TextureType type{};

        Texture() = default;
        Texture(const aiString& str, TextureType type);
    };

    enum class ShadingModel {
        PBR
    };

    struct Material {
        struct PBRMaterial {
            glm::vec4 baseColorFactor{};
            float metallicFactor{};
            float roughnessFactor{};

            int albedoTexture{};
            int metallicRoughnessTexture{};
            int normalTexture{};
            int occlusionTexture{};
            int emissiveTexture{};
        };
        PBRMaterial pbr{};
        ShadingModel shadingModel{};
    };

    struct MeshIndices {
        const unsigned* indices{};
        unsigned numIndices{};

        auto begin() const {
            return indices;
        }

        auto end() const {
            return indices + numIndices;
        }
    };

    struct ModelLoadingCache {
        const aiScene* scene{};

        std::string directory{};

        /* Nodes */
        FlattenedAssimpHierarchy* hierarchy{};
        std::string* hierarchyNodeNames{};
        Transform* hierarchyLocalTransforms{};
        unsigned numNodes{};

        std::unordered_map<std::string_view, unsigned> nodeNameToHierarchyIndex{};

        /* Animations */
        ModelAnimation* animations{};
        size_t numAnimations{};

        /* Bones */
        Bone* bones{};
        int* nodeToBoneIndex{}; /* -1 if no Bone */
        int boneCounter = 1;

        /* Render Data */
        Vertex* vertices{};
        size_t numVertices{};

        unsigned* indices{};
        size_t numIndices{};

        Mesh* meshes{};
        MeshIndices* nodeToMeshIndices{};
        size_t numMeshes{};

        std::vector<Texture> textures;

        Material* materials{};
        size_t numMaterials{};

        size_t getTotalMeshIndices() const {
            size_t total = 0;
            for (size_t i = 0; i < numNodes; ++i) {
                total += nodeToMeshIndices[i].numIndices;
            }
            return total;
        }

        ModelLoadingCache() = default;
        ModelLoadingCache(const ModelLoadingCache&) = delete;
        ModelLoadingCache(ModelLoadingCache&& other) noexcept
        : scene(other.scene), hierarchy(other.hierarchy),
            hierarchyNodeNames(other.hierarchyNodeNames),
            hierarchyLocalTransforms(other.hierarchyLocalTransforms),
            numNodes(other.numNodes),
            nodeNameToHierarchyIndex(std::move(other.nodeNameToHierarchyIndex)),
            animations(other.animations), numAnimations(other.numAnimations),
            bones(other.bones), nodeToBoneIndex(other.nodeToBoneIndex),
            boneCounter(other.boneCounter), vertices(other.vertices),
            numVertices(other.numVertices), indices(other.indices), numIndices(other.numIndices),
            meshes(other.meshes), nodeToMeshIndices(other.nodeToMeshIndices), numMeshes(other.numMeshes),
            textures(std::move(other.textures)), materials(other.materials), numMaterials(other.numMaterials)
        {
            other.scene = nullptr;
            other.hierarchy = nullptr;
            other.hierarchyNodeNames = nullptr;
            other.hierarchyLocalTransforms = nullptr;
            other.numNodes = 0;

            other.nodeNameToHierarchyIndex.clear();

            other.animations = nullptr;
            other.numAnimations = 0;

            other.bones = nullptr;
            other.nodeToBoneIndex = nullptr;
            other.boneCounter = 0;

            other.vertices = nullptr;
            other.numVertices = 0;

            other.indices = nullptr;
            other.numIndices = 0;

            other.meshes = nullptr;
            other.nodeToMeshIndices = nullptr;
            other.numMeshes = 0;

            other.textures.clear();

            other.materials = nullptr;
            other.numMaterials = 0;
        }

        ModelLoadingCache& operator=(const ModelLoadingCache&) = delete;

        ModelLoadingCache& operator=(ModelLoadingCache&& other) noexcept {
            if (this == &other) return *this;
            this->~ModelLoadingCache();
            return *new (this) ModelLoadingCache(std::move(other));
        }

        ~ModelLoadingCache();

        std::span<const Mesh> getMeshes() const { return std::span(meshes, numMeshes); }
    };
private:
    void flattenImplRecursive(FlattenedAssimpHierarchy* parent, const aiNode* assimpParent, int& nextIdx);
    void flattenAssimpHierarchy();

    void loadNodeNames();
    void loadNodeLocalTransforms();

    void loadAnimations();

    void loadMeshVertices(const Mesh *myMesh, const aiMesh *assimpMesh);
    void loadMeshBoneData(Mesh* myMesh, const aiMesh* assimpMesh);

    void normalizeBones() const;

    void loadMeshes();
    void loadMeshBones();

    unsigned tryLoadTexture(const aiString& path, Texture texture);
    void loadMaterials();

    ModelLoadingCache cache{};
    ModelCookingParams params{};
public:
    ModelCooking(ModelCookingParams params) : params(params) {}

    ModelCooking(const ModelCooking&) = delete;
    ModelCooking(ModelCooking&&) = delete;
    ModelCooking& operator=(const ModelCooking&) = delete;
    ModelCooking& operator=(ModelCooking&&) = delete;

    ~ModelCooking() {
        discard();
    }

    static ModelCooking create(ModelCookingParams params) {
        return {params};
    }

    ModelLoadingCache cook();

    void discard();
};