#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "ImageDesc.h"
#include "ImageAPI.h"

class IMAGEAPI Image : public ImageDescriptor {
    ImageHandle myHandle{};
    void* myPixels{};
public:
    Image() = default;

    Image(const int width, const int height, const ImageChannels ch, const PixelType pixelType, void* pixels, ImageHandle&& handle)
        : ImageDescriptor(width, height, ch, pixelType), myHandle(std::move(handle)), myPixels(pixels) {}

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept
    : ImageDescriptor(other), myHandle(std::move(other.myHandle)), myPixels(other.myPixels) {
        other.myPixels = nullptr;
        other.myHandle = {};
    }

    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            myHandle.destroy();
            myPixels = other.myPixels;

            other.myPixels = nullptr;

            ImageDescriptor::operator=(other);
            myHandle = std::move(other.myHandle);
        }
        return *this;
    }

    ~Image() {
        myHandle.destroy();
    }

    int mipmaps() const {
        return 1 + static_cast<int>(std::floor(std::log2(std::max(width(), height()))));
    }

    DynamicPixel load(const unsigned u, const unsigned v) const;

    void store(const unsigned u, const unsigned v, const DynamicPixel& pixel);

    Image slice(const int x, const int y, const int w, const int h) const {
        const int fw = std::min(w, width() - x);
        const int fh = std::min(h, height() - y);

        void* pixels = new char[pixel_stride(pixelType()) * fw * fh];

        std::memcpy(pixels, (char*)this->pixels() + (y * width() + x) * pixel_stride(pixelType()), fw * fh * pixel_stride(pixelType()));

        return Image(fw, fh, channels(), pixelType(), pixels, ImageHandle::create([](const void* p) {
            delete[] static_cast<const char*>(p);
        }));
    }

    void* pixels() {
        return myPixels;
    }

    const void* pixels() const {
        return myPixels;
    }
};

class ImageView : public ImageDescriptor {
    const void* myPixels{};
public:
    ImageView(const ImageDescriptor& desc, const void* pixels) : ImageDescriptor(desc), myPixels(pixels) {}

    const void* pixels() const {
        return myPixels;
    }

    DynamicPixel load(unsigned u, unsigned v) const;
};