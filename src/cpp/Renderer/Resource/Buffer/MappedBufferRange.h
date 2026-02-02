#pragma once
#include <cassert>
#include <memory/Span.h>

template <typename T>
class TMappedBufferRange;

class AMappedBufferRange {
protected:
    void* region{};
    size_t sizeInBytes{};
public:
    AMappedBufferRange() = default;
    AMappedBufferRange(void* region, size_t sizeInBytes) : region(region), sizeInBytes(sizeInBytes) {}

    size_t size() const {
        return sizeInBytes;
    }

    template <typename T>
    TMappedBufferRange<T> as() {
        return TMappedBufferRange<T>(region, sizeInBytes);
    }

    bool empty() const { return sizeInBytes == 0;  }

    void* data() {
        return region;
    }

    const void* data() const {
        return region;
    }
};

template <typename T>
class TMappedBufferRange : public AMappedBufferRange {
public:
    TMappedBufferRange(void* region, const size_t sizeInBytes) : AMappedBufferRange(region, sizeInBytes) {}

    size_t sizeInBytes() const {
        return AMappedBufferRange::size();
    }

    size_t size() const {
        return sizeInBytes() / sizeof(T);
    }

    T& operator [] (const size_t index) {
        assert(index < size());
        return data()[index];
    }

    mem::range<T> span(size_t offset = 0, size_t count = -1) {
        count = count == -1 ? size() - offset : count;
        size_t fOffset = std::min(offset, size());
        size_t fCount = std::min(count, size() - offset);
        return mem::make_range(data() + fOffset, fCount);
    }

    T* data() { return static_cast<T*>(region); }
    const T* data() const { return static_cast<const T*>(region); }
};
