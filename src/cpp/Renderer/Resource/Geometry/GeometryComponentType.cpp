#include "GeometryComponentType.h"

#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>

#include "GeometryBuilder.h"
#include "VertexLayout.h"
#include "GeometryQuery.h"

static constexpr auto DEBUG_PRINT_VERTICES = false;

void * GeometryStagingBuffer::Ops::createVertexPtr(const void *vertices, const size_t count, const mem::typeindex type) {
    void* v = arena.allocate(type, count);
     memcpy(v, vertices, count * type.size());

    if constexpr (DEBUG_PRINT_VERTICES) {
        for (auto& vertex : mem::make_range((float*)vertices, count)) {
            std::cout << vertex << ",";
        }
        std::cout << std::endl;
        for (auto& vertex : mem::make_range((float*)v, count)) {
            std::cout << vertex << ",";
        }
        std::cout << std::endl;
    }
    return v;
}

void * GeometryStagingBuffer::Ops::createIndexPtr(const void *indices, size_t count, mem::typeindex type) {
    void* i = arena.allocate(type, count);
    memcpy(i, indices, count * type.size());
    return i;
}

void GeometryStagingBuffer::Ops::createGeometry(GeometryKey key, void *v, void *i, size_t numV, size_t numI, mem::typeindex vType,
    IndexType iType, const VertexLayout* layout) {
    newGeometry.emplace_back(key, v, i, numV, numI, vType, iType, layout);
}

void GeometryStagingBuffer::reset() {
    for (auto& tlops : ops) {
        tlops.newGeometry.release();
        tlops.arena.reset_compact();
    }
}

GeometryResourceStorage::GeometryResourceStorage() {
    geometries.emplace_back();

}

void GeometryResourceStorage::synchronize() {
    geometries.reserve(nextKey);
    while (geometries.size() != geometries.capacity()) {
        geometries.emplace_back();
    }

    for (auto& [arena, newGeometries] : stagingBuffer.ops) {
        for (auto& geometry : newGeometries) {
            createGeometry(geometry.geometry, geometry.vertices,
                geometry.indices, geometry.numVertices,
                geometry.numIndices, geometry.verticesType,
                geometry.indexType, geometry.layout
            );
        }
    }

    stagingBuffer.reset();
}

GeometryKey GeometryResourceStorage::createGeometry(GeometryKey geometry, const void *vertices, const void *indices,
                                                  size_t numVertices, size_t numIndices, mem::typeindex verticesType, const IndexType indexType,
                                                  const VertexLayout* vertexLayout, const ComponentIndex cIndex)
{

    Geometry geom;
    geom.verticesType = verticesType;
    geom.indexType = indexType;
    geom.numIndices = numIndices;
    geom.numVertices = numVertices;

    size_t stride = 0;
    for (auto& attr : vertexLayout->getAttributes()) {
        stride += attr.bytes;
    }

    glGenVertexArrays(1, &geom.gpuVAO);
    glBindVertexArray(geom.gpuVAO);

    unsigned vObj = 0;
    glGenBuffers(1, &vObj);
    glBindBuffer(GL_ARRAY_BUFFER, vObj);
    glBufferData(GL_ARRAY_BUFFER, geom.numVertices * stride, vertices, GL_STATIC_DRAW);

    geom.gpuVBO = vObj;

    if (indices) {
        unsigned iObj = 0;
        glGenBuffers(1, &iObj);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iObj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, geom.numIndices * index_type_size(geom.indexType), indices, GL_STATIC_DRAW);

        geom.gpuEBO = iObj;
    }

    size_t offset = 0;

    for (size_t i = 0; i < vertexLayout->size(); i++) {
        const auto& [components, bytes, type] = vertexLayout->getAttributes()[i];

        if (type == VertexAttributeType::FLOAT) {
            glVertexAttribPointer(
                i, components,
                opengl_enum_cast(type),
                GL_FALSE,
                stride,
                reinterpret_cast<void*>(offset)
            );
        } else {
            glVertexAttribIPointer(
                i, components, opengl_enum_cast(type), stride, reinterpret_cast<void*>(offset)
            );
        }

        glEnableVertexAttribArray(i);

        offset += bytes;
    }

    const size_t totalVertexBytes = geom.numVertices * geom.verticesType.size();
    const size_t numDistinctVertices = totalVertexBytes / stride;

    if (totalVertexBytes % stride != 0) { // alignment padding or attributes dont match
        assert(false);
    }

    geom.numDistinctVertices = numDistinctVertices;

    geometries[geometry.index()] = GeometryHeader{std::move(geom), vertexLayout};

    if constexpr (DEBUG_PRINT_VERTICES) {
        const char* cVertices = static_cast<const char*>(vertices);
        for (size_t i = 0; i < totalVertexBytes; i += stride) {
            size_t offset = 0;

            std::cout << "Vertex [" << i / stride << "]: \n";
            for (auto& attr : vertexLayout->getAttributes()) {
                const char* byte = cVertices + i + offset;

                std::cout << "{";

                std::cout << std::setprecision(2);
                for (size_t j = 0; j < attr.bytes; j += sizeof(float)) {
                    std::cout << *(const float*)(byte + j);

                    if (j < attr.bytes - sizeof(float)) {
                        std::cout << ", ";
                    }
                }
                offset += attr.bytes;
                std::cout << "}, ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    }

    glBindVertexArray(0);
    return geometry;
}

void GeometryResourceStorage::ensure(GeometryKey max) {
    geometries.reserve(max.index() + 1);
    while (geometries.size() != geometries.capacity()) {
        geometries.emplace_back();
    }
}

GeometryFactory::GeometryFactory(Renderer &renderer) : type(&renderer.getGeometryStorage()) {
}

GeometryKey GeometryFactory::loadGeometry(const GeometryDescriptor &descriptor) const {
    auto& [vertices, indices, vCount, iCount, vType, iType, layout, lHash] = descriptor;

    const GeometryKey key = type->newGeometry();

    void* v = type->stagingBuffer.ops.local().createVertexPtr(vertices, vCount, vType);
    void* i = indices ? type->stagingBuffer.ops.local().createIndexPtr(indices, iCount, iType) : nullptr;

    IndexType indexType = {};

    if (indices) {
        if (iType.is<unsigned short>()) {
            indexType = IndexType::UINT16;
        } else if (iType.is<unsigned int>()) {
            indexType = IndexType::UINT32;
        } else {
            assert(false);
        }
    }
    type->stagingBuffer.ops.local().createGeometry(key, v, i, vCount, iCount, vType, indexType, layout);
    return key;
}

/*
 *
 *
 */

GeometryQuery::GeometryQuery(Renderer &renderer) : type(&renderer.getGeometryStorage()) {}

const Geometry & GeometryQuery::getGeometry(GeometryKey key) const {
    if (!isKeyValid(key)) {
        assert(false);
    }
    return type->geometries[key.index()].geometry;
}

DrawRange GeometryQuery::getFullDrawRange(GeometryKey key) const {
    if (!isKeyValid(key)) return {};

    const auto& header = type->geometries[key.index()];
    const auto& geometry = header.geometry;

    DrawRange draw;
    draw.baseVertex = 0;
    draw.firstIndex = 0;
    draw.indexCount = geometry.numIndices ? geometry.numIndices : geometry.numDistinctVertices;
    return draw;
}

bool GeometryQuery::isKeyValid(GeometryKey key) const {
    return key.index() < type->geometries.size() && key != NULL_GEOMETRY_KEY;
}