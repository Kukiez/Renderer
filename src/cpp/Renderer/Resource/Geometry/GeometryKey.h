#pragma once
#include "../ResourceKey.h"

class GeometryKey : public UnsignedKeyBase {
public:
    using UnsignedKeyBase::UnsignedKeyBase;
};

static constexpr auto NULL_GEOMETRY_KEY = GeometryKey{};