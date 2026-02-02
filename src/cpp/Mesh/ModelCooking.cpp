#include "Mesh/ModelCooking.h"

#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtx/matrix_decompose.hpp>
#include "Animation/ModelAnimation.h"

template <typename Fn>
void forEachAssimpNode(const aiNode* node, Fn&& fn) {
    fn(node);

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        forEachAssimpNode(node->mChildren[i], fn);
    }
}

static auto convertAssimpToGLM(auto& glm, auto& assimp) {
    glm[0][0] = assimp.a1; glm[1][0] = assimp.a2; glm[2][0] = assimp.a3; glm[3][0] = assimp.a4;
    glm[0][1] = assimp.b1; glm[1][1] = assimp.b2; glm[2][1] = assimp.b3; glm[3][1] = assimp.b4;
    glm[0][2] = assimp.c1; glm[1][2] = assimp.c2; glm[2][2] = assimp.c3; glm[3][2] = assimp.c4;
    glm[0][3] = assimp.d1; glm[1][3] = assimp.d2; glm[2][3] = assimp.d3; glm[3][3] = assimp.d4;
}


ModelCooking::Texture::Texture(const aiString &str, TextureType type): path(str.C_Str()), type(type) {}

ModelCooking::ModelLoadingCache::~ModelLoadingCache() {
    delete[] hierarchy;
    delete[] hierarchyNodeNames;
    delete[] hierarchyLocalTransforms;
    delete[] animations;
    delete[] bones;
    delete[] nodeToBoneIndex;
    delete[] vertices;
    delete[] indices;
    delete[] meshes;
    delete[] materials;

    for (unsigned i = 0; i <numNodes; ++i) {
        delete[] nodeToMeshIndices[i].indices;
    }
    delete[] nodeToMeshIndices;
}

void ModelCooking::flattenImplRecursive(FlattenedAssimpHierarchy* parent, const aiNode* assimpNode, int& nextIdx) {
    int currIdx = nextIdx;

    parent->firstChild = currIdx;
    parent->numChildren = static_cast<int>(assimpNode->mNumChildren);

    for (unsigned i = 0; i < assimpNode->mNumChildren; ++i) {
        cache.hierarchy[currIdx + i].node = assimpNode->mChildren[i];
    }

    nextIdx += static_cast<int>(assimpNode->mNumChildren);

    for (unsigned i = 0; i < assimpNode->mNumChildren; ++i) {
        flattenImplRecursive(&cache.hierarchy[currIdx + i],
            assimpNode->mChildren[i], nextIdx
        );
    }
}

void ModelCooking::flattenAssimpHierarchy() {
    unsigned totalAssimpSum = 0;

    forEachAssimpNode(cache.scene->mRootNode, [&](const aiNode*) {
        ++totalAssimpSum;
    });

    cache.numNodes = totalAssimpSum;

    cache.hierarchy = new FlattenedAssimpHierarchy[totalAssimpSum];

    int next = 1;
    cache.hierarchy[0].node = cache.scene->mRootNode;

    flattenImplRecursive(cache.hierarchy, cache.scene->mRootNode, next);
}

void ModelCooking::loadNodeNames() {
    cache.hierarchyNodeNames = new std::string[cache.numNodes];

    for (unsigned i = 0; i < cache.numNodes; ++i) {
        auto& node = cache.hierarchy[i];
        cache.hierarchyNodeNames[i] = std::string(node.node->mName.C_Str(), node.node->mName.length);

        cache.nodeNameToHierarchyIndex.emplace(cache.hierarchyNodeNames[i], i);
    }
}

void ModelCooking::loadNodeLocalTransforms() {
    cache.hierarchyLocalTransforms = new Transform[cache.numNodes];

    for (size_t i = 0; i < cache.numNodes; ++i) {
        auto& node = cache.hierarchy[i];

        glm::mat4 glmTransform;
        convertAssimpToGLM(glmTransform, node.node->mTransformation);

        glm::vec3 position, scale, skew;
        glm::vec4 perspective;
        glm::quat q;
        glm::decompose(glmTransform, scale, q, position, skew, perspective);

        cache.hierarchyLocalTransforms[i] = Transform(position, scale, q);
    }
}

void ModelCooking::loadAnimations() {
    const auto scene = cache.scene;

    cache.animations = new ModelAnimation[scene->mNumAnimations];

    for (unsigned a = 0; a < scene->mNumAnimations; a++) {
        const auto& anim = *scene->mAnimations[a];

        const double speed = anim.mTicksPerSecond != 0 ? anim.mTicksPerSecond : 25;

        ModelAnimation& animation = cache.animations[a];
        animation = ModelAnimation(anim.mName.C_Str());
        for (unsigned c = 0; c < anim.mNumChannels; c++) {
            const auto& chan = anim.mChannels[c];

            const auto node = cache.nodeNameToHierarchyIndex.find(chan->mNodeName.C_Str());

            if (node == cache.nodeNameToHierarchyIndex.end()) {
                std::cout << "Animation Node not found?: " << chan->mNodeName.C_Str() << std::endl;
                continue;
            }

            const unsigned nodeIndex = node->second;

            for (unsigned i = 0; i < chan->mNumPositionKeys; ++i) {
                const auto& posKey = chan->mPositionKeys[i];
                animation.addTranslation(nodeIndex, posKey.mTime / speed, glm::vec3(posKey.mValue.x, posKey.mValue.y, posKey.mValue.z));
            }
            for (unsigned i = 0; i < chan->mNumScalingKeys; ++i) {
                const auto& posKey = chan->mScalingKeys[i];
                animation.addScale(nodeIndex, posKey.mTime / speed, glm::vec3(posKey.mValue.x, posKey.mValue.y, posKey.mValue.z));
            }
            for (unsigned i = 0; i < chan->mNumRotationKeys; ++i) {
                const auto& posKey = chan->mRotationKeys[i];
                animation.addRotation(nodeIndex, posKey.mTime / speed, glm::quat(posKey.mValue.w, posKey.mValue.x, posKey.mValue.y, posKey.mValue.z));
            }
        }
        std::cout << "Animation Loaded: " << scene->mAnimations[a]->mName.C_Str() << ": " << scene->mAnimations[a]->mDuration << std::endl;
    }
}

void ModelCooking::loadMeshVertices(const Mesh* myMesh, const aiMesh *assimpMesh) {
    auto vertices = assimpMesh->mVertices;
    auto normals = assimpMesh->mNormals;
    auto tangents = assimpMesh->mTangents;
    auto bitangents = assimpMesh->mBitangents;
    
    for (unsigned i = 0; i < assimpMesh->mNumVertices; i++) {
        Vertex& vertex = cache.vertices[myMesh->vertexOffset + i];
        
        vertex.position = glm::vec3(vertices[i].x, vertices[i].y, vertices[i].z);
        vertex.normal = glm::vec3(normals[i].x, normals[i].y, normals[i].z);
        vertex.normal = glm::normalize(vertex.normal);

        if (assimpMesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = assimpMesh->mTextureCoords[0][i].x;
            vec.y = assimpMesh->mTextureCoords[0][i].y;
            vertex.uv0 = vec;
        }
        vertex.tangent = glm::vec3(tangents[i].x, tangents[i].y, tangents[i].z);
        vertex.bitangent = glm::vec3(bitangents[i].x, bitangents[i].y, bitangents[i].z);
        vertex.tangent = glm::normalize(vertex.tangent);
        vertex.bitangent = glm::normalize(vertex.bitangent);
    }

    for (unsigned i = 0; i < assimpMesh->mNumFaces; i++) {
        const aiFace face = assimpMesh->mFaces[i];
        for (unsigned j = 0; j < face.mNumIndices; j++) {
            cache.indices[myMesh->indexOffset + i * 3 + j] = face.mIndices[j];
        }
    }
}

void ModelCooking::loadMeshBoneData(Mesh *myMesh, const aiMesh *assimpMesh) {
    int& boneCount = cache.boneCounter;

    for (unsigned boneIndex = 0; boneIndex < assimpMesh->mNumBones; ++boneIndex) {
        auto bone = assimpMesh->mBones[boneIndex];

        std::string_view boneName = bone->mName.C_Str();
        auto boneNode = cache.nodeNameToHierarchyIndex.find(boneName);

        if (boneNode == cache.nodeNameToHierarchyIndex.end()) {
            std::cout << "Bone not found?: " << boneName << std::endl;
            assert(false);
        }

        int& nodeBoneIndex = cache.nodeToBoneIndex[boneNode->second];

        int boneID;
        if (nodeBoneIndex == -1) {
            glm::mat4 offsetMatrix;
            convertAssimpToGLM(offsetMatrix, bone->mOffsetMatrix);

            cache.bones[boneCount].nodeID = static_cast<int>(boneNode->second);
            cache.bones[boneCount].offsetMatrix = offsetMatrix;

            boneID = boneCount;
            boneCount++;

            nodeBoneIndex = boneID;
        } else {
            boneID = nodeBoneIndex;
        }

        const auto weights = bone->mWeights;
        const unsigned numWeights = bone->mNumWeights;

        for (unsigned weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
            unsigned vertexId = weights[weightIndex].mVertexId + myMesh->vertexOffset;
            float weight = weights[weightIndex].mWeight;

            auto& v = cache.vertices[vertexId];

            bool found = false;

            for (int i = 0; i < 4; ++i) {
                if (v.boneIndices[i] == -1) {
                    v.boneIndices[i] = boneID;
                    v.boneWeights[i] = weight;
                    found = true;
                    break;
                }
            }

            if (found) continue;

            int minIndex = 0;
            for (int i = 1; i < 4; ++i)
                if (v.boneWeights[i] < v.boneWeights[minIndex])
                    minIndex = i;

            if (weight > v.boneWeights[minIndex]) {
                v.boneIndices[minIndex] = boneID;
                v.boneWeights[minIndex] = weight;
            }
        }
    }
}

void ModelCooking::normalizeBones() const {
    // for (unsigned b = 0; b < cache.numNodes; ++b) {
    //     auto& boneIdx = cache.nodeToBoneIndex[b];
    //
    //     if (boneIdx == -1) {
    //         boneIdx = 0;
    //     }
    // }

    for (unsigned i = 0; i < cache.numVertices; ++i) {
        auto& vertex = cache.vertices[i];

        const float total = vertex.boneWeights[0]
            + vertex.boneWeights[1]
            + vertex.boneWeights[2]
            + vertex.boneWeights[3];

        if (total > 0.0f) {
            vertex.boneWeights[0] /= total;
            vertex.boneWeights[1] /= total;
            vertex.boneWeights[2] /= total;
            vertex.boneWeights[3] /= total;
        } else {
            vertex.boneIndices[0] = 0;
            vertex.boneWeights[0] = 1.0f;

            for (int j = 1; j < 4; ++j) {
                vertex.boneIndices[j] = 0;
                vertex.boneWeights[j] = 0.0f;
            }
        }
    }
}

void ModelCooking::loadMeshes() {
    const auto scene = cache.scene;

    unsigned totalVertices = 0;
    unsigned totalIndices = 0;

    cache.meshes = new Mesh[scene->mNumMeshes];
    cache.numMeshes = scene->mNumMeshes;

    for (unsigned m = 0; m < scene->mNumMeshes; m++) {
        const auto& mesh = scene->mMeshes[m];

        Mesh& myMesh = cache.meshes[m];

        myMesh.vertexOffset = totalVertices;
        myMesh.indexOffset = totalIndices;
        myMesh.numIndices = mesh->mNumFaces * 3;

        totalVertices += mesh->mNumVertices;
        totalIndices += mesh->mNumFaces * 3;
    }

    cache.vertices = new Vertex[totalVertices];
    cache.numVertices = totalVertices;
    cache.indices = new unsigned[totalIndices];
    cache.numIndices = totalIndices;

    for (unsigned m = 0; m < scene->mNumMeshes; m++) {
        const auto& mesh = scene->mMeshes[m];

        Mesh& myMesh = cache.meshes[m];

        myMesh.materialIndex = mesh->mMaterialIndex;
        loadMeshVertices(&myMesh, mesh);
    }

    cache.nodeToMeshIndices = new MeshIndices[cache.numNodes];

    for (unsigned i = 0; i < cache.numNodes; ++i) {
        auto aiNode = cache.hierarchy[i].node;

        unsigned* nodeMeshIndices = nullptr;
        if (aiNode->mNumMeshes) {
            nodeMeshIndices = new unsigned[aiNode->mNumMeshes];
            for (unsigned m = 0; m < aiNode->mNumMeshes; ++m) {
                nodeMeshIndices[m] = aiNode->mMeshes[m];
            }
        }

        cache.nodeToMeshIndices[i] = MeshIndices(nodeMeshIndices, aiNode->mNumMeshes);
    }
}

void ModelCooking::loadMeshBones() {
    const auto scene = cache.scene;

    cache.bones = new Bone[cache.numNodes + 1];
    cache.nodeToBoneIndex = new int[cache.numNodes];

    cache.bones[0].offsetMatrix = glm::mat4(1.0f);

    for (unsigned i = 0; i < cache.numNodes; ++i) {
        cache.nodeToBoneIndex[i] = -1;
    }

    for (unsigned m = 0; m < scene->mNumMeshes; m++) {
        const auto& mesh = scene->mMeshes[m];

        Mesh& myMesh = cache.meshes[m];
        loadMeshBoneData(&myMesh, mesh);
    }
    normalizeBones();
}

unsigned ModelCooking::tryLoadTexture(const aiString &path, Texture texture) {
    if (path.length == 0) return -1;

    const auto it = std::ranges::find_if(cache.textures, [&](const Texture& t) { return t.path == path.C_Str(); });
    if (it != cache.textures.end()) return static_cast<unsigned>(it - cache.textures.begin());

    texture.path = cache.directory + "/" + texture.path;
    cache.textures.emplace_back(texture);
    return static_cast<unsigned>(cache.textures.size()) - 1;
}

aiString getTexturePath(const aiMaterial* material, const aiTextureType texType) {
    aiString path;
    if (material->GetTexture(texType, 0, &path) != AI_SUCCESS) return aiString("");
    return path;
}

void ModelCooking::loadMaterials() {
    const auto scene = cache.scene;

    cache.numMaterials = scene->mNumMaterials;
    cache.materials = new Material[cache.numMaterials];

    for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
        const auto& material = scene->mMaterials[i];

        auto diffuse = getTexturePath(material, aiTextureType_BASE_COLOR);
        auto normal = getTexturePath(material, aiTextureType_NORMAL_CAMERA);
        auto emissive = getTexturePath(material, aiTextureType_EMISSION_COLOR);
        auto ao = getTexturePath(material, aiTextureType_AMBIENT_OCCLUSION);
        auto gltfMetallicRoughness = getTexturePath(material, aiTextureType_GLTF_METALLIC_ROUGHNESS);

        auto diffIdx = tryLoadTexture(diffuse, Texture(diffuse, TextureType::DIFFUSE));
        auto normalIdx = tryLoadTexture(normal, Texture(normal, TextureType::NORMAL));
        auto emissiveIdx = tryLoadTexture(emissive, Texture(emissive, TextureType::EMISSIVE));
        auto aoIdx = tryLoadTexture(ao, Texture(ao, TextureType::AMBIENT_OCCLUSION));
        auto mrIdx = tryLoadTexture(gltfMetallicRoughness, Texture(gltfMetallicRoughness, TextureType::GLTF_METALLIC_ROUGHNESS));

        Material::PBRMaterial pbr;
        pbr.albedoTexture = diffIdx;
        pbr.normalTexture = normalIdx;
        pbr.emissiveTexture = emissiveIdx;
        pbr.occlusionTexture = aoIdx;
        pbr.metallicRoughnessTexture = mrIdx;

        cache.materials[i] = Material(pbr);
    }
}

ModelCooking::ModelLoadingCache ModelCooking::cook() {
    static constexpr auto AI_PROCESS_FLAGS = aiProcess_Triangulate |
                           aiProcess_GenSmoothNormals |
                           aiProcess_CalcTangentSpace |
                           aiProcess_JoinIdenticalVertices |
                           aiProcess_ImproveCacheLocality |
                           aiProcess_OptimizeMeshes |
                           aiProcess_RemoveRedundantMaterials |
                           aiProcess_SortByPType;

    Assimp::Importer importer;
    auto scene = importer.ReadFile(params.path.data(), AI_PROCESS_FLAGS);

    cache.scene = scene;
    cache.directory = params.path.substr(0, params.path.find_last_of('/'));

    flattenAssimpHierarchy();
    loadNodeNames();

    loadNodeLocalTransforms();

    loadAnimations();

    loadMeshes();

    loadMeshBones();

    loadMaterials();

    for (unsigned i = 0; i < cache.numNodes; ++i) {
        auto& boneIdx = cache.nodeToBoneIndex[i];

        std::cout << cache.hierarchyNodeNames[i] << " Bone: " << boneIdx << std::endl;
    }

    return std::move(cache);
}

void ModelCooking::discard() {
    cache = ModelLoadingCache();
}
