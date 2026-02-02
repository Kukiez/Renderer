#pragma once
#include "ECS/Global/FrameAllocator.h"
#include <unordered_set>

namespace ecs {
    using frame_string = std::basic_string<char, std::char_traits<char>, FrameAllocatorAdaptor<char>>;

    template <typename T>
    using frame_vector = mem::vector<T, FrameAllocatorAdaptor<T>>;

    template <typename TK, typename TV, typename Hash, typename Eq>
    using frame_hashmap = std::unordered_map<TK, TV, Hash, Eq, FrameAllocatorAdaptor<std::pair<const TK, TV>>>;

    template <typename TK, typename Hash, typename Eq>
    using frame_hashset = std::unordered_set<TK, Hash, Eq, FrameAllocatorAdaptor<TK>>;
}

class LevelFrame {
    FrameAllocator* allocator;
public:
    LevelFrame(FrameAllocator& allocator) : allocator(&allocator) {}

    template <typename T>
    auto allocateVector(const size_t elements) {
        return ecs::frame_vector<T>(allocator, elements);
    }

    template <typename TK, typename TV, typename Hash, typename Eq>
    auto allocateMap(const size_t capacity) {
        ecs::frame_hashmap<TK, TV, Hash, Eq> map(allocator);
        map.reserve(capacity);
        return map;
    }

    template <typename TK, typename Hash, typename Eq>
    auto allocateHashSet(size_t capacity) {
        ecs::frame_hashset<TK, Hash, Eq> set(allocator);
        set.reserve(capacity);
        return set;
    }

    auto allocateString(size_t capacity) const {
         auto str = ecs::frame_string{allocator};
        str.reserve(capacity);
         return std::move(str);
    }

    auto allocateString(std::string_view chars) const {
        auto str = ecs::frame_string{allocator};
        str = chars;
        return std::move(str);
    }

    void* allocateRaw(const size_t bytes) const {
        return allocator->allocateUnmanaged(mem::type_info_of<char>, bytes);
    }

    template <typename T, typename... Args>
    T* allocateType(Args&&... args) {
        void* mem = allocator->allocate(mem::type_info_of<T>, 1);
        new (mem) T(std::forward<Args>(args)...);
        return static_cast<T*>(mem);
    }
};
