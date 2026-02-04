#pragma once
#include <Imath/half.h>

enum class PixelType {
    BYTE,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INTEGER,
    UNSIGNED_INT,
    HALF_FLOAT,
    FLOAT,
    DOUBLE,
    FLOAT_16 = HALF_FLOAT,
    FLOAT_32 = FLOAT,
    FLOAT_64 = DOUBLE,
    NUM_PIXEL_TYPES
};

template<PixelType>
struct PixelCppType;

template<>
struct PixelCppType<PixelType::BYTE> {
    using type = int8_t;
};

template<>
struct PixelCppType<PixelType::UNSIGNED_BYTE> {
    using type = uint8_t;
};

template<>
struct PixelCppType<PixelType::SHORT> {
    using type = int16_t;
};

template<>
struct PixelCppType<PixelType::UNSIGNED_SHORT> {
    using type = uint16_t;
};

template<>
struct PixelCppType<PixelType::INTEGER> {
    using type = int32_t;
};

template<>
struct PixelCppType<PixelType::UNSIGNED_INT> {
    using type = uint32_t;
};

template<>
struct PixelCppType<PixelType::HALF_FLOAT> {
    using type = half;   // OpenEXR half
};

template<>
struct PixelCppType<PixelType::FLOAT> {
    using type = float;
};

template<>
struct PixelCppType<PixelType::DOUBLE> {
    using type = double;
};

template<PixelType PT>
using PixelCppType_t = PixelCppType<PT>::type;

constexpr uint8_t pixel_stride(const PixelType type) noexcept {
    switch (type) {
        case PixelType::BYTE:           return 1;
        case PixelType::UNSIGNED_BYTE:  return 1;
        case PixelType::SHORT:          return 2;
        case PixelType::UNSIGNED_SHORT: return 2;
        case PixelType::INTEGER:        return 4;
        case PixelType::UNSIGNED_INT:   return 4;
        case PixelType::HALF_FLOAT:     return 2;
        case PixelType::FLOAT:          return 4;
        case PixelType::DOUBLE:         return 8;
        default:                        return 0;
    }
}

enum class ImageChannels : uint8_t {
    R = 1,
    RG,
    RGB,
    RGBA
};

enum class ImageChannel {
    R,
    RED = R,
    G,
    GREEN,
    B,
    BLUE,
    A,
    ALPHA,
};



class ImageDescriptor {
    ImageChannels myChannels{};
    int myWidth{}, myHeight{};
    PixelType myPixelType{};
public:
    ImageDescriptor() = default;

    ImageDescriptor(const int width, const int height, const ImageChannels ch, const PixelType pixelType)
        :  myChannels(ch),  myWidth(width), myHeight(height), myPixelType(pixelType) {}

    int mipmaps() const {
        return 1 + static_cast<int>(std::floor(std::log2(std::max(myWidth, myHeight))));
    }

    int width() const {
        return myWidth;
    }

    int height() const {
        return myHeight;
    }

    PixelType pixelType() const {
        return myPixelType;
    }

    ImageChannels channels() const {
        return myChannels;
    }
};

struct ImageHandle {
    void* handle = nullptr;

    using DestroyHandle = void(*)(void*);
    DestroyHandle destroyHandle = nullptr;

    ImageHandle() = default;

    explicit ImageHandle(void* handle, DestroyHandle destroy)
        : handle(handle), destroyHandle(destroy) {}

    ImageHandle(const ImageHandle&) = delete;
    ImageHandle& operator=(const ImageHandle&) = delete;

    ImageHandle(ImageHandle&& other) noexcept
    : handle(other.handle), destroyHandle(other.destroyHandle) {
        other.handle = nullptr;
        other.destroyHandle = nullptr;
    }

    ImageHandle& operator=(ImageHandle&& other) noexcept {
        if (this != &other) {
            handle = other.handle;
            destroyHandle = other.destroyHandle;
            other.handle = nullptr;
            other.destroyHandle = nullptr;
        }
        return *this;
    }

    template <typename Destructor>
    requires std::is_empty_v<std::decay_t<Destructor>>
    static ImageHandle create(Destructor) {
        using TDestructor = std::decay_t<Destructor>;
        return ImageHandle(reinterpret_cast<void*>(1), [](void*) {
            TDestructor();
        });
    }

    template <typename New_AllocatedObject>
    static ImageHandle create(New_AllocatedObject* obj) {
        return ImageHandle(obj, [](void* o) {
            delete static_cast<New_AllocatedObject*>(o);
        });
    }

    bool destroy() {
        if (handle) {
            destroyHandle(handle);
            handle = nullptr;
            destroyHandle = nullptr;
            return true;
        }
        return false;
    }

    operator bool() const {
        return handle != nullptr;
    }
};

struct DynamicPixel {
    alignas(8) char r[8]{};
    alignas(8) char g[8]{};
    alignas(8) char b[8]{};
    alignas(8) char a[8]{};

    DynamicPixel() = default;

    template <typename T>
    explicit DynamicPixel(const T r, const T g = {}, const T b = {}, const T a = {}) {
        memcpy(this->r, &r, sizeof(T));
        memcpy(this->g, &g, sizeof(T));
        memcpy(this->b, &b, sizeof(T));
        memcpy(this->a, &a, sizeof(T));
    }

    char* operator [] (const ImageChannel channel) {
        switch (channel) {
            case ImageChannel::R: return r;
            case ImageChannel::G: return g;
            case ImageChannel::B: return b;
            case ImageChannel::A: return a;
            default: std::unreachable();
        }
    }
};