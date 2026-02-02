#include "ImageLoader.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <algorithm>
#include <stb/stb_image.h>
#include <filesystem>
#include <unordered_map>
#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb/stb_image_write.h"
#include <ImfRgbaFile.h>   // for Imf::Rgba, Imf::RgbaInputFile
#include <ImfArray.h>      // for Imf::Array2D
#include <half.h>
#include <Util/enum_bit.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;

template <typename T>
void flipImpl(Image& image) {
    T* pixels = static_cast<T*>(image.pixels());

    const int width = image.width();
    const int height = image.height();
    const int channels = static_cast<int>(image.channels());
    const int rowSize = width * channels;

    for (int y = 0; y < height / 2; ++y) {
        T* rowTop = pixels + y * rowSize;
        T* rowBottom = pixels + (height - 1 - y) * rowSize;

        for (int i = 0; i < rowSize; ++i) {
            std::swap(rowTop[i], rowBottom[i]);
        }
    }
}


Image & ImageProcessor::flip(Image &image) {
    switch (image.pixelType()) {
        case ::PixelType::DOUBLE:
            flipImpl<double>(image);
            break;
        case ::PixelType::FLOAT:
            flipImpl<float>(image);
        case ::PixelType::HALF_FLOAT:
            flipImpl<half>(image);
            break;
        case ::PixelType::BYTE:
        case ::PixelType::UNSIGNED_BYTE:
            flipImpl<unsigned char>(image);
            break;
        case ::PixelType::UNSIGNED_INT:
            flipImpl<unsigned int>(image);
            break;
        default:
            assert(false);
    }
    return image;
}

Image ImageProcessor::merge(const SourceChannel red, SourceChannel green, SourceChannel blue, SourceChannel alpha, const ::PixelType type) {
    const uint8_t channels = [&] {
        if (alpha) return 4;
        if (blue) return 3;
        if (green) return 2;
        return 1;
    }();

    using DimensionGetFn = int(Image::*)() const;

    auto GetImageDimension = [&](const DimensionGetFn dimension) {
        int sizes[] = {
            red   ? (red.image->*dimension)()   : -1,
            green ? (green.image->*dimension)() : -1,
            blue  ? (blue.image->*dimension)()  : -1,
            alpha ? (alpha.image->*dimension)() : -1
        };
        return *std::ranges::max_element(sizes);
    };

    const int width = GetImageDimension(&Image::width);
    const int height = GetImageDimension(&Image::height);

    if (width == -1 || height == -1) {
        return {};
    }

    const auto stride = pixel_stride(type);

    const size_t totalBytes = static_cast<size_t>(width) * height * channels * stride;
    void* pixels = operator new (totalBytes, std::align_val_t{8});


    auto CopyImageImpl = [&]<typename PixelType, int Channels>() {
        auto* dst = static_cast<PixelType*>(pixels);

        if (stride != sizeof(PixelType)) assert(false);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                size_t dstIndex = (y * width + x) * Channels;

                if constexpr (Channels >= 1) {
                    PixelType& r = dst[dstIndex + 0];
                    auto srcR = red->load(x, y);
                    std::memcpy(&r, srcR[red.channel], sizeof(PixelType));
                }
                if constexpr (Channels >= 2) {
                    PixelType& g = dst[dstIndex + 1];
                    auto srcG = green->load(x, y);
                    std::memcpy(&g, srcG[green.channel], sizeof(PixelType));
                }
                if constexpr (Channels >= 3) {
                    PixelType& b = dst[dstIndex + 2];
                    auto srcB = blue->load(x, y);
                    std::memcpy(&b, srcB[blue.channel], sizeof(PixelType));
                }
                if constexpr (Channels >= 4) {
                    PixelType& a = dst[dstIndex + 3];
                    auto srcA = alpha->load(x, y);
                    std::memcpy(&a, srcA[alpha.channel], sizeof(PixelType));
                }
            }
        }
    };

    auto CopyImage = [&]<typename PixelType>(const int channels) {
        switch (channels) {
            case 1: CopyImageImpl.operator()<PixelType, 1>(); break;
            case 2: CopyImageImpl.operator()<PixelType, 2>(); break;
            case 3: CopyImageImpl.operator()<PixelType, 3>(); break;
            case 4: CopyImageImpl.operator()<PixelType, 4>(); break;
            default: break;
        }
    };

    switch (type) {
        case ::PixelType::DOUBLE:
            CopyImage.operator()<double>(channels);
            break;
        case ::PixelType::FLOAT:
            CopyImage.operator()<float>(channels);
            break;
        case ::PixelType::HALF_FLOAT:
            CopyImage.operator()<half>(channels);
            break;
        case ::PixelType::BYTE:
            CopyImage.operator()<char>(channels);
            break;
        case ::PixelType::UNSIGNED_INT:
            CopyImage.operator()<unsigned int>(channels);
            break;
        case ::PixelType::UNSIGNED_SHORT:
            CopyImage.operator()<unsigned short>(channels);
            break;
        case ::PixelType::UNSIGNED_BYTE:
            CopyImage.operator()<unsigned char>(channels);
            break;
        default:
            assert(false);
    }

    return Image(width, height, static_cast<ImageChannels>(channels), type, pixels, ImageHandle::create([](void* pixels) {
        operator delete(pixels, std::align_val_t{8});
    }));
}

Image ImageLoader::load_EXR16F(std::string_view path) {
    try {
        RgbaInputFile file(path.data());
        const Imath_3_2::Box2i dw = file.dataWindow();

        const int width  = dw.max.x - dw.min.x + 1;
        const int height = dw.max.y - dw.min.y + 1;

        Array2D<Rgba>& pixels = *new Array2D<Rgba>;
        pixels.resizeErase(height, width);

        file.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
        file.readPixels(dw.min.y, dw.max.y);

        const auto rawData = reinterpret_cast<half*>(&pixels[0][0]);

        const auto channels = file.channels();

        ImageChannels imgChannels;

        if (channels == WRITE_RGB) {
            imgChannels = ImageChannels::RGB;
        } else if (channels == WRITE_RGBA) {
            imgChannels = ImageChannels::RGBA;
        } else {
            imgChannels = ImageChannels::RGBA;
        }

        std::cout << "Loaded: " << width << " x " << height << std::endl;

        return Image(width, height, ImageChannels::RGBA, ::PixelType::HALF_FLOAT, rawData, ImageHandle::create(&pixels));
    } catch (std::exception& e) {
        std::cerr << "Failed to load EXR file: " << path << std::endl;
        std::cerr << e.what() << std::endl;
        return {};
    }
}


Image ImageLoader::loadImpl(const std::string_view file, const ImageLoadParams &params) {
    const bool flipV = (params.options & ImageLoadOptions::FLIP_VERTICALLY) == ImageLoadOptions::FLIP_VERTICALLY;

    if (file.ends_with(".exr")) {
        Image image = load_EXR16F(file); // Add Params

        if (flipV) {
            ImageProcessor::flip(image);
        }
        return image;
    }
    int width, height, channels;

    const int forcedChannels = params.channels == ImageLoadChannels::AUTO ? 0 : static_cast<int>(params.channels);

    if (flipV) {
        stbi_set_flip_vertically_on_load(1);
    }

    unsigned char* image = stbi_load(file.data(), &width, &height, &channels, forcedChannels);

    if (!image) {
        std::cerr << "[ERROR] Failed to load texture: " << file << std::endl;
        return {};
    }

    if (flipV) {
        stbi_set_flip_vertically_on_load(0);
    }
    static constexpr auto STB_FREE_FN = [](void* ptr) { stbi_image_free(static_cast<unsigned char*>(ptr)); };
    return Image(width, height, static_cast<ImageChannels>(channels), ::PixelType::UNSIGNED_BYTE, image, ImageHandle::create(STB_FREE_FN));
}

Image ImageLoader::empty(const unsigned width, const unsigned height, const ::PixelType type, ImageChannels channels, const DynamicPixel &value) {
    size_t stride = pixel_stride(type);
    const size_t totalBytes = static_cast<size_t>(width) * height * static_cast<int>(channels) * pixel_stride(type);
    void* pixels = operator new (totalBytes, std::align_val_t{8});

    const int ch = static_cast<int>(channels);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t dstIndex = (y * width + x) * ch * stride;

            char* dst = static_cast<char*>(pixels) + dstIndex;
            std::memcpy(dst, value.r, stride);

            if (ch >= 2) {
                dst = static_cast<char*>(pixels) + dstIndex + stride;
                std::memcpy(dst, value.g, stride);
            }
            if (ch >= 3) {
                dst = static_cast<char*>(pixels) + dstIndex + stride * 2;
                std::memcpy(dst, value.b, stride);
            }
            if (ch == 4) {
                dst = static_cast<char*>(pixels) + dstIndex + stride * 3;
                std::memcpy(dst, value.a, stride);
            }
        }
    }
    return Image(width, height, channels, type, pixels, ImageHandle::create([](void* pixels) {
        operator delete(pixels, std::align_val_t{8});
    }));
}

Image ImageLoader::procedural(unsigned width, unsigned height, ::PixelType type, ImageChannels channels) {
    auto imgPixels = new char[pixel_stride(type) * width * height * static_cast<int>(channels)]{};
    return Image(width, height, channels, type, imgPixels, ImageHandle::create(STD_CHAR_FREE_FN));
}

void pixel_cast(const void* src, ::PixelType srcType, void* dst, ::PixelType dstType, bool normalize = false) {
    auto loadAsDouble = [](const void* src, ::PixelType type) -> double {
        switch (type) {
            case ::PixelType::BYTE:           return *(const int8_t*)src;
            case ::PixelType::UNSIGNED_BYTE:  return *(const uint8_t*)src;
            case ::PixelType::SHORT:          return *(const int16_t*)src;
            case ::PixelType::UNSIGNED_SHORT: return *(const uint16_t*)src;
            case ::PixelType::INTEGER:        return *(const int32_t*)src;
            case ::PixelType::UNSIGNED_INT:   return *(const uint32_t*)src;
            case ::PixelType::HALF_FLOAT:     return float(*static_cast<const half *>(src));
            case ::PixelType::FLOAT:          return *(const float*)src;
            case ::PixelType::DOUBLE:         return *(const double*)src;
            default:                        return 0.0;
        }
    };

    auto loadNormalized = [&](const void* src, ::PixelType type) {
        switch (type) {
            case ::PixelType::UNSIGNED_BYTE:  return *(uint8_t*)src / 255.0;
            case ::PixelType::UNSIGNED_SHORT: return *(uint16_t*)src / 65535.0;
            default: return loadAsDouble(src, type);
        }
    };

    auto storeFromDouble = [](void* dst, ::PixelType type, double v) {
        switch (type) {
            case ::PixelType::BYTE:           *(int8_t*)dst   = (int8_t)v;   break;
            case ::PixelType::UNSIGNED_BYTE:  *(uint8_t*)dst  = (uint8_t)v;  break;
            case ::PixelType::SHORT:          *(int16_t*)dst  = (int16_t)v;  break;
            case ::PixelType::UNSIGNED_SHORT: *(uint16_t*)dst = (uint16_t)v; break;
            case ::PixelType::INTEGER:        *(int32_t*)dst  = (int32_t)v;  break;
            case ::PixelType::UNSIGNED_INT:   *(uint32_t*)dst = (uint32_t)v; break;
            case ::PixelType::HALF_FLOAT:     *(half*)dst     = half(v);     break;
            case ::PixelType::FLOAT:          *(float*)dst    = (float)v;   break;
            case ::PixelType::DOUBLE:         *(double*)dst   = v;          break;
            default: break;
        }
    };
    double value = normalize ? loadNormalized(src, srcType) : loadAsDouble(src, srcType);
    storeFromDouble(dst, dstType, value);
}


Image & ImageLoader::fill(Image &dstView, const void *pixels, ::PixelType pixelsType) {
    if (dstView.pixelType() != pixelsType) {
        ImageDescriptor srcDest(dstView.width(), dstView.height(), dstView.channels(), pixelsType);

        ImageView srcView(srcDest, pixels);

        for (unsigned u = 0; u < dstView.width(); ++u) {
            for (unsigned v = 0; v < dstView.height(); ++v) {
                auto srcPixel = srcView.load(u, v);

                int channels = static_cast<int>(dstView.channels());

                DynamicPixel dstPixel;

                for (int i = 0; i < channels; ++i) {
                    pixel_cast(srcPixel[static_cast<ImageChannel>(i)], srcView.pixelType(), dstPixel[static_cast<ImageChannel>(i)], dstView.pixelType());
                }
                dstView.store(u, v, dstPixel);
            }
        }
    } else {
        std::memcpy(dstView.pixels(), pixels, dstView.width() * dstView.height() * pixel_stride(pixelsType) * static_cast<int>(dstView.channels()));
    }
    return dstView;
}

bool ImageLoader::save(Image &img, std::string_view filename) {
    stbi_write_png(filename.data(), img.width(), img.height(), static_cast<int>(img.channels()), img.pixels(), (int)img.channels() * img.width() * pixel_stride(img.pixelType()));
    return false;
}
