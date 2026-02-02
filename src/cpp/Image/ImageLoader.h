#pragma once
#include <algorithm>
#include <Image/Image.h>

enum class ImageLoadOptions {
    NONE = 0,
    FLIP_VERTICALLY = 1 << 0,
};

enum class ImageLoadChannels {
    AUTO = -1,
    R,
    RG,
    RGB,
    RGBA
};

struct ImageLoadParams {
    ImageLoadOptions options = ImageLoadOptions::NONE;
    ImageLoadChannels channels = ImageLoadChannels::AUTO;
};

struct SourceChannel {
    const Image* image{};
    ImageChannel channel{};

    operator bool() const {
        return image != nullptr;
    }

    const Image* operator -> () const {
        return image;
    }
};

class ImageProcessor {
public:
    static Image& flip(Image& image);
    static Image merge(SourceChannel red, SourceChannel green, SourceChannel blue, SourceChannel alpha, const PixelType pixelType);
};

class ImageLoader {
    static Image load_EXR16F(std::string_view path);

    template <typename Loader>
    struct ImageFreeInfo {
        Loader loader;
    };

    static Image loadImpl(const std::string_view file, const ImageLoadParams& params);
public:
    static constexpr auto STD_CHAR_FREE_FN = [](void* img) {
        delete[] static_cast<char*>(img);
    };

    static Image empty(const unsigned width, const unsigned height, PixelType type, ImageChannels channels, const DynamicPixel &value);

    static Image load(const std::string_view file, const ImageLoadParams& params = {}) {
        return loadImpl(file, params);
    }

    static Image load(const std::string_view file, const ImageLoadOptions options) {
        ImageLoadParams params;
        params.options = options;
        return loadImpl(file, params);
    }

    static Image procedural(unsigned width, unsigned height, PixelType type, ImageChannels channels);

    static Image& fill(Image& dstView, const void* pixels, PixelType pixelsType);

    static bool save(Image& img, std::string_view filename);
};