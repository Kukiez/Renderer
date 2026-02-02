#pragma once
#include <Renderer/Resource/ResourceKey.h>


class ShaderComponentType;

class ShaderKey : public UnsignedKeyBase {
public:
    constexpr ShaderKey() : UnsignedKeyBase(0) {}

    template <typename T>
    explicit ShaderKey(const T id) : UnsignedKeyBase(id) {}
};

static constexpr auto NULL_SHADER_KEY = ShaderKey{};