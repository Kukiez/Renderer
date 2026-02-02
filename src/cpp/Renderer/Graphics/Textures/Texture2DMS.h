#pragma once
#include "Texture.h"

class Texture2DMS : public TextureBase {
    int mySamples = 0;
public:
    Texture2DMS() = default;

    Texture2DMS(int width, int height, int samples, TextureFormat format);

    int samples() const { return mySamples; }
};
