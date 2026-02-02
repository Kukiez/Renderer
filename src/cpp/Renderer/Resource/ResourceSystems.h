#pragma once
#include "Geometry/GeometryBuilder.h"
#include "Geometry/GeometryQuery.h"
#include "Shader/ShaderFactory.h"
#include "Shader/ShaderQuery.h"
#include "Texture/TextureFactory.h"
#include "Texture/TextureQuery.h"

class ResourceFactory : public TextureFactory, public GeometryFactory, public ShaderFactory {
public:
    ResourceFactory(LevelContext& level) : TextureFactory(level),  GeometryFactory(level), ShaderFactory(level) {}
};

class ResourceQuery : public TextureQuery, public GeometryQuery, public ShaderQuery {
public:
    ResourceQuery(Level& level) : TextureQuery(level), GeometryQuery(level), ShaderQuery(level) {
    }

    ResourceQuery(LevelContext& level) : TextureQuery(level), GeometryQuery(level), ShaderQuery(level) {
    }

};