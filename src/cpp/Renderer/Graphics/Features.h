#pragma once
#include <string_view>
#include <memory/type_info.h>

class Renderer;
class GraphicsContext;

struct GraphicsAllocator {
protected:
    virtual void* allocate(mem::typeindex type, size_t count) = 0;
public:
    virtual ~GraphicsAllocator() = default;

    template <typename T>
    T* allocate(const size_t count = 1) {
        static_assert(std::is_trivially_destructible_v<T>, "GraphicsAllocator only supports trivial types, For non trivial types use allocateNonTrivial");
        return static_cast<T *>(allocate(mem::type_info::of<T>(), count));
    }

    template <typename T>
    T* allocateAligned(const size_t count, const size_t aligned) {
        auto type = *mem::type_info::of<T>();
        type.align = aligned;
        return static_cast<T *>(allocate(mem::typeindex(&type), count));
    }

    template <typename T>
    T* allocateAsTrivial(const size_t count) {
        return static_cast<T *>(allocate(mem::type_info::of<T>(), count));
    }
};

template <typename T, bool IsRef, typename VSize = size_t>
class GraphicsVectorBase {
    using TDataPtr = std::conditional_t<IsRef, T*&, T*>;
    using TSize = std::conditional_t<IsRef, VSize&, VSize>;

    GraphicsAllocator* allocator;
    TDataPtr dataPtr;
    TSize dataSize;
    TSize dataCapacity;
    double wantedGrowth;
public:
    GraphicsVectorBase(GraphicsAllocator* allocator, VSize capacity = 0, double wantedGrowth = 2)
    requires (!IsRef)
        : allocator(allocator), dataPtr(allocator->allocate<T>(capacity)), dataSize(0), dataCapacity(capacity), wantedGrowth(wantedGrowth) {}

    GraphicsVectorBase(GraphicsAllocator* allocator, T*& dataPtr, VSize& dataSize, VSize& dataCapacity, double wantedGrowth = 2)
    requires IsRef
        : allocator(allocator), dataPtr(dataPtr), dataSize(dataSize), dataCapacity(dataCapacity), wantedGrowth(wantedGrowth) {}

    T& operator[](const VSize index) {
        return dataPtr[index];
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        static_assert(std::is_trivially_destructible_v<T>, "GraphicsVectorView only supports trivial types");
        if (dataSize == dataCapacity) {
            VSize newCapacity = dataCapacity ?
                static_cast<VSize>(dataCapacity * wantedGrowth) : 8;

            T* newData = allocator->allocate<T>(newCapacity);
            std::memcpy(newData, dataPtr, sizeof(T) * dataSize);
            dataPtr = newData;
            dataCapacity = newCapacity;
        }
        return *new (&dataPtr[dataSize++]) T(std::forward<Args>(args)...);
    }

    bool empty() const { return dataSize == 0; }

    VSize size() const { return dataSize; }
    VSize capacity() const { return dataCapacity; }

    T& back() {
        assert(!empty());
        return dataPtr[dataSize - 1];
    }

    auto begin() const {
        return dataPtr;
    }

    auto end() const {
        return dataPtr + dataSize;
    }
};

template <typename T, typename VSize = size_t>
using GraphicsVector = GraphicsVectorBase<T, false, VSize>;

template <typename T, typename VSize = size_t>
using GraphicsVectorView = GraphicsVectorBase<T, true, VSize>;

template <typename T>
class TGraphicsAllocator {
    GraphicsAllocator* allocator;
public:
    constexpr TGraphicsAllocator(GraphicsAllocator* allocator) : allocator(allocator) {}

    using value_type = T;

    T* allocate(const size_t count = 1) {
        return allocator->allocate<T>(count);
    }

    static constexpr void deallocate(T* ptr, const size_t count = 1) {}
};

struct GraphicsFeature {
    virtual bool validate(GraphicsContext& ctx) { return true; }

    virtual void push(GraphicsContext& ctx) = 0;

    virtual void pop(GraphicsContext& ctx) {}

    virtual std::string_view name() const = 0;
};