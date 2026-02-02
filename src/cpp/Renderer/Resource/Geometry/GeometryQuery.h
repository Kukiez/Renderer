#pragma once
#include "GeometryKey.h"

struct Geometry;
struct DrawRange;

class GeometryQuery {
    GeometryResourceStorage* type;
public:
    GeometryQuery(GeometryResourceStorage& type) : type(&type) {}
    GeometryQuery(Renderer& renderer);

    const Geometry& getGeometry(GeometryKey key) const;

    DrawRange getFullDrawRange(GeometryKey key) const;

    bool isKeyValid(GeometryKey key) const;
};