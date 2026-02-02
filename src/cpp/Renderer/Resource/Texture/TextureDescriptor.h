#pragma once
#include <Image/ImageArray.h>
#include <Image/ImageLoader.h>

struct TextureDescriptor2D {
    TextureCreateParams params{};

    struct FromImage {
        Image image{};
    };

    struct FromFile {
        std::string path{};
        ImageLoadOptions options{};
    };

    struct FromEmpty {
        int width, height;
    };

    std::variant<FromEmpty, FromImage, FromFile> image = FromEmpty{};

    TextureDescriptor2D() = default;

    TextureDescriptor2D(const TextureCreateParams& params, const int width, const int height) : params(params) {
        image.emplace<FromEmpty>(width, height);
    }

    TextureDescriptor2D(const TextureCreateParams& params, Image&& image) : params(params) {
        this->image.emplace<FromImage>(std::move(image));
    }

    TextureDescriptor2D(const std::string& path, const TextureCreateParams& params, const ImageLoadOptions& options = {}) : params(params) {
        image.emplace<FromFile>(path, options);
    }

    void from(Image& fromImage) {
        image.emplace<FromImage>(std::move(fromImage));
    }

    void from(const std::string& path, const ImageLoadOptions& options = {}) {
        image.emplace<FromFile>(path, options);
    }

    void from(int width, int height) {
        image.emplace<FromEmpty>(width, height);
    }
};

struct EmptyCubemapDescriptor2D {
    TextureCreateParams params{};
    int width{}, height{};

    EmptyCubemapDescriptor2D() = default;
    EmptyCubemapDescriptor2D(const TextureCreateParams& params, const int width, const int height) : params(params), width(width), height(height) {}
};

struct TextureDescriptor2DMS {
    int samples{};
    int width{}, height{};
    TextureFormat format{};

    TextureDescriptor2DMS() = default;
    TextureDescriptor2DMS(int samples, int width, int height, TextureFormat format) : samples(samples), width(width), height(height), format(format) {}
};

struct TextureDescriptor2DArray {
    ImageArray images{};
    TextureCreateParams params{};
};