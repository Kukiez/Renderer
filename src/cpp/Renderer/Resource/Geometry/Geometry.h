#pragma once
#include <memory/type_info.h>

enum class IndexType {
    UINT16, UINT32
};

constexpr size_t index_type_size(IndexType type) {
    return type == IndexType::UINT16 ? sizeof(uint16_t) : sizeof(uint32_t);
}

struct Geometry {
    unsigned gpuVAO{};
    unsigned gpuVBO{};
    unsigned gpuEBO{};

    void* vertices = nullptr;
    void* indices = nullptr;

    mem::typeindex verticesType{};
    IndexType indexType{};

    size_t numVertices = 0; // total T's in vertices
    size_t numIndices = 0;

    size_t numDistinctVertices = 0; // total Vertices (  sizeof(vertices) / Layout.Stride )

    bool hasIndices() const { return numIndices > 0; }
};