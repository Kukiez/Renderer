#pragma once
#include <span>
#include "ImageDesc.h"
class ImageArray : public ImageDescriptor {
public:
    struct Slice {
        void* pixels{};
        int width{}, height{};
        int xOffset{}, yOffset{};
    };
private:
    Slice* slices{};
    size_t numSlices{};
    std::function<void(Slice* slices, size_t numSlices)> deleter{};
public:
    ImageArray() = default;

    static ImageArray empty(const int width, const int height, const PixelType pixelType, const ImageChannels channels) {
        ImageArray result{};
        static_cast<ImageDescriptor&>(result) = ImageDescriptor(width, height, channels, pixelType);
        return result;
    }

    template <typename Deleter>
    ImageArray(const ImageDescriptor desc, Slice* slices, const size_t numSlices, Deleter&& deleter) : ImageDescriptor(desc), slices(slices), numSlices(numSlices), deleter(std::forward<Deleter>(deleter)) {}

    ImageArray(const ImageArray&) = delete;
    ImageArray& operator=(const ImageArray&) = delete;

    ImageArray(ImageArray&& other) noexcept : slices(other.slices), numSlices(other.numSlices), deleter(std::move(other.deleter)) {
        other.slices = nullptr;
        other.numSlices = 0;
    }

    ImageArray& operator=(ImageArray&& other) noexcept {
        if (this != &other) {
            deleter = std::move(other.deleter);
            slices = other.slices;
            numSlices = other.numSlices;
            other.slices = nullptr;
            other.numSlices = 0;
        }
        return *this;
    }

    ~ImageArray() {
        if (deleter) {
            deleter(slices, numSlices);
        }
    }

    std::span<const Slice> getSlices() const {
        return std::span<const Slice>(slices, numSlices);
    }

    size_t depth() const {
        return numSlices;
    }

    bool hasData() const {
        return slices != nullptr;
    }

    Slice operator [] (const size_t index) const {
        return index < numSlices ? slices[index] : Slice{};
    }

    void validateAndClampSlices() {
        const int texW = width();
        const int texH = height();

        for (size_t i = 0; i < numSlices; ++i) {
            Slice& s = slices[i];

            if (!s.pixels || s.width <= 0 || s.height <= 0) {
                s.width  = 0;
                s.height = 0;
                continue;
            }

            if (s.xOffset < 0) {
                s.width += s.xOffset;
                s.xOffset = 0;
            }
            if (s.yOffset < 0) {
                s.height += s.yOffset;
                s.yOffset = 0;
            }

            if (s.xOffset + s.width > texW) {
                s.width = texW - s.xOffset;
            }
            if (s.yOffset + s.height > texH) {
                s.height = texH - s.yOffset;
            }

            if (s.width <= 0 || s.height <= 0) {
                s.width  = 0;
                s.height = 0;
                s.pixels = nullptr;
            }
        }
    }
};
