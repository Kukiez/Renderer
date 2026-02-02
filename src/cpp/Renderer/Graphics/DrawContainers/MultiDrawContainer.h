#pragma once

class MultiDrawContainer final : public GraphicsDrawContainer {
    GraphicsAllocator* allocator;
    GraphicsDrawContainer** drawContainers = nullptr;
    uint32_t numDrawContainers = 0;
    uint32_t capDrawContainers = 0;

    void addDrawContainer(GraphicsDrawContainer* pass);

public:
    MultiDrawContainer(GraphicsAllocator* allocator, const size_t reserve = 2) : allocator(allocator), capDrawContainers(reserve) {
        if (capDrawContainers == 0) {
            capDrawContainers = 2;
        }
        drawContainers = allocator->allocate<GraphicsDrawContainer*>(capDrawContainers);
    }

    template <IsGraphicsDrawContainer T, typename... Args>
    T& addDrawContainer(Args&&... args) {
        T* container = allocator->allocate<T>();
        new (container) T(allocator, std::forward<Args>(args)...);
        addDrawContainer(container);
        return *container;
    }

    void draw(GraphicsContext &ctx) override;
};
