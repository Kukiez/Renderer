#pragma once
#include "ShaderKey.h"

class ShaderClass;
class ShaderProgram;
class Renderer;

class ShaderQuery {

    ShaderComponentType* type;
public:
    ShaderQuery(Renderer& renderer);

    const ShaderProgram& getShaderProgram(ShaderKey key) const;

    const ShaderClass& findShaderClass(std::string_view name) const;
};
