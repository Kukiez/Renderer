#pragma once
#include <array>

#include "ImageDesc.h"

enum class CubemapFace : uint8_t {
    POSITIVE_X,
    NEGATIVE_X,
    POSITIVE_Y,
    NEGATIVE_Y,
    POSITIVE_Z,
    NEGATIVE_Z
};

static constexpr std::array CubemapFaces = {
    CubemapFace::POSITIVE_X,
    CubemapFace::NEGATIVE_X,
    CubemapFace::POSITIVE_Y,
    CubemapFace::NEGATIVE_Y,
    CubemapFace::POSITIVE_Z,
    CubemapFace::NEGATIVE_Z
};

class CubemapImage : public ImageDescriptor {
public:
    class Face {
        friend class CubemapImage;
        void* myPixels{};
    public:
        Face() = default;

        Face(void* pixels) : myPixels(pixels) {}

        void* pixels() {
            return myPixels;
        }

        const void* pixels() const {
            return myPixels;
        }
    };
private:
    std::array<Face, 6> myFaces{};
    ImageHandle myHandle{};
public:
    CubemapImage(void** pixels, const CubemapFace* faces, ImageHandle&& handle) : myHandle(std::move(handle)) {
        for (int i = 0; i < 6; ++i) {
            myFaces[static_cast<int>(faces[i])] = Face(pixels[i]);
        }
    }

    CubemapImage(const CubemapImage&) = delete;
    CubemapImage& operator=(const CubemapImage&) = delete;

    CubemapImage(CubemapImage&& other) noexcept
    : ImageDescriptor(other), myFaces(other.myFaces), myHandle(std::move(other.myHandle)) {
        other.myFaces = {};
    }

    CubemapImage& operator=(CubemapImage&& other) noexcept {
        if (this != &other) {
            myFaces = other.myFaces;
            myHandle.destroy();

            myHandle = std::move(other.myHandle);
            ImageDescriptor::operator=(other);

            other.myFaces = {};
        }
        return *this;
    }

    ~CubemapImage() {
        clear();
    }

    void clear() {
        if (myHandle.destroy()) {
            for (auto& face : myFaces) {
                face.myPixels = nullptr;
            }
        }
    }

    const Face& getFace(const CubemapFace face) const {
        return myFaces[static_cast<int>(face)];
    }
};