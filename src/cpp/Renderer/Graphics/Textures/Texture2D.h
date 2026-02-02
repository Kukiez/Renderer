#pragma once
#include "Texture.h"

class ImageArray;
class Image;

class Texture2D : public TextureBase {
public:
    Texture2D() = default;

    Texture2D(int width, int height, const TextureCreateParams& params);

    Texture2D(const Image &image, const TextureCreateParams &params);
};

class Texture2DArray : public TextureBase {
public:
    Texture2DArray() = default;

    Texture2DArray(int width, int height, int depth, const TextureCreateParams& params);

    Texture2DArray(const ImageArray &imageArray, const TextureCreateParams &params);
};