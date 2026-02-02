#pragma once
#include <memory/type_info.h>
#include "GeometryKey.h"
#include "VertexLayout.h"

class Level;
struct LevelContext;

class GeometryResourceStorage;

struct GeometryDescriptor {
    const void* vertices{};
    const void* indices{};
    size_t vCount{};
    size_t iCount{};
    mem::typeindex vType{};
    mem::typeindex iType{};

    const VertexLayout* layout{};
    VertexLayoutHash layoutHash{};

    template <typename Vertices, typename Indices, typename Layout>
    requires std::ranges::contiguous_range<Vertices> && std::ranges::contiguous_range<Indices>
    GeometryDescriptor(Vertices&& vertices, Indices&& indices, Layout) {
        static constexpr Layout layout;

        this->vertices = std::ranges::data(vertices);
        this->indices = std::ranges::data(indices);
        vCount = std::ranges::size(vertices);
        iCount = std::ranges::size(indices);
        vType = mem::type_info_of<std::ranges::range_value_t<Vertices>>;
        iType = mem::type_info_of<std::ranges::range_value_t<Indices>>;
        this->layout = &layout.Layout;
        layoutHash = layout.Hash;
    }

    template <typename Vertices, typename Layout>
    requires std::ranges::contiguous_range<Vertices>
    GeometryDescriptor(Vertices&& vertices, Layout) {
        static constexpr Layout layout;

        this->vertices = std::ranges::data(vertices);
        vCount = std::ranges::size(vertices);
        vType = mem::type_info_of<std::ranges::range_value_t<Vertices>>;
        this->layout = &layout.Layout;
        layoutHash = layout.Hash;
    }
};

class GeometryFactory : public EntityComponentFactory {
    friend class VertexLayoutBuilder;

    GeometryResourceStorage* type;
public:
    explicit GeometryFactory(GeometryResourceStorage & geometry) : type(&geometry) {}
    GeometryFactory(Renderer& renderer);

    GeometryKey loadGeometry(const GeometryDescriptor& descriptor) const;
};