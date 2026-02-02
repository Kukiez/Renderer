#pragma once
#include <Renderer/Graphics/GraphicsContext.h>
#include "VertexLayoutKey.h"
#include "VertexLayout.h"
#include "GeometryKey.h"
#include "Geometry.h"

struct GeometryHeader {
    Geometry geometry{};
    const VertexLayout* layout{};
};

struct GeometryStagingBuffer {
    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;

    struct NewGeometry {
        GeometryKey geometry{};

        void* vertices = 0;
        void* indices = 0;
        size_t numVertices = 0;
        size_t numIndices = 0;

        mem::typeindex verticesType{};
        IndexType indexType{};

        const VertexLayout* layout{};
    };

    struct NewStaticVertexLayout {
        VertexLayoutKey layoutKey{};
        const VertexLayout* layout{};
        VertexLayoutHash hash{};
    };

    struct NewDynamicVertexLayout {
        VertexLayoutKey layoutKey{};
        const AttributeInfo* attributes{};
        size_t numLocations{};
    };

    struct LinkGeometryToName {
        GeometryKey geometry{};
        ComponentIndex id{};
    };

    struct Ops {
        Arena arena = Arena(0.01 * 1024 * 1024);
        mem::vector<NewGeometry, Arena::Adaptor<NewGeometry>> newGeometry{&arena};

        void* createVertexPtr(const void* vertices, size_t count, mem::typeindex type);
        void* createIndexPtr(const void* indices, size_t count, mem::typeindex type);

        void createGeometry(GeometryKey key, void *v, void *i, size_t numV, size_t numI, mem::typeindex vType, IndexType iType, const VertexLayout
                            *layout);
    };
    ThreadLocal<Ops> ops;

    void reset();
};

struct NamedGeometryField {
    GeometryKey geometry{};
};

class GeometryResourceStorage {
    friend class GeometryFactory;
    friend class GeometryQuery;

    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;
    Arena layoutArena = Arena(0.01 * 1024 * 1024);

    mem::vector<GeometryHeader> geometries;

    std::atomic<unsigned> nextKey = 1;

    GeometryStagingBuffer stagingBuffer{};
public:
    GeometryResourceStorage();

    GeometryResourceStorage(const GeometryResourceStorage&) = delete;
    GeometryResourceStorage& operator=(const GeometryResourceStorage&) = delete;

    void synchronize();

    GeometryKey newGeometry() {
        return GeometryKey{nextKey++};
    }

    void freeKey(const GeometryKey key) {}
    void freeKey(const VertexLayoutKey key) {}

    GeometryKey createGeometry(GeometryKey geometry, const void *vertices, const void *indices,
        size_t numVertices, size_t numIndices, mem::typeindex verticesType, IndexType indexType,
        const VertexLayout* vertexLayout, const ComponentIndex cIndex = {});

    void ensure(GeometryKey max);
};