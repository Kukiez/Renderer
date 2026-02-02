#pragma once
#include "Texture.h"

class CubemapImage;

class Cubemap : public TextureBase{
public:
    Cubemap() = default;

    Cubemap(const int width, const int height, const TextureCreateParams& params);

    Cubemap(const CubemapImage& image,  const TextureCreateParams& params);
};
