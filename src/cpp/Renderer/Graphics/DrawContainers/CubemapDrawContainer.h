#pragma once
#include <Image/CubemapImage.h>

class CubemapDrawContainer final : public GraphicsDrawContainer {
    GraphicsAllocator* allocator;
    GraphicsDrawContainer* faceContainers[6]{};
public:
    CubemapDrawContainer(GraphicsAllocator* allocator) : allocator(allocator) {}

    template <typename T, typename... Args>
    T& usingDrawContainer(const CubemapFace face, Args&&... args) {
        T* container = allocator->allocate(sizeof(T), alignof(T));
        new (container) T(allocator, std::forward<Args>(args)...);
        faceContainers[static_cast<int>(face)] = container;
        return *container;
    }

    void draw(GraphicsContext &ctx) override;
};
