#pragma once
#include "Component.h"
#include <memory/vector.h>
#include "TypeUUID.h"

struct ComponentMapIteratorSentinelEnd {};

template <typename V, bool ReturnBothTypeAndValue = true>
class ComponentMapIterator {
    ComponentKind kind;
    std::pair<V, bool>* first;
    int index;
    int length;
public:
    ComponentMapIterator() = default;
    ComponentMapIterator(ComponentKind kind, std::pair<V, bool>* first, int index, int length) : kind(kind), first(first), index(index), length(length) {}

    bool operator == (const ComponentMapIteratorSentinelEnd) const {
        return index == length;
    }
    bool operator != (const ComponentMapIteratorSentinelEnd) const {
        return index != length;
    }

    std::pair<TypeUUID, V&> operator* () requires (ReturnBothTypeAndValue) {
        return std::forward_as_tuple(TypeUUID::of(kind, index), first[index].first);
    }

    V& operator * () requires (!ReturnBothTypeAndValue) {
        return first[index].first;
    }

    void increment() {
        if (index == length) return;
        ++index;
        while (index != length && !first[index].second) {
            ++index;
        }
    }
    ComponentMapIterator& operator ++() {
        increment();
        return *this;
    }

    ComponentMapIterator operator ++ (int) {
        auto d = *this;
        increment();
        return d;
    }
};

template <typename V, bool ReturnBothTypeAndValue = true>
struct ComponentMapIteratable {
    ComponentKind kind;
    std::pair<V, bool>* first;
    int index;

    auto begin() {
        return ComponentMapIterator<V, ReturnBothTypeAndValue>{
            kind, first, 0, index
        };
    }

    auto end() {
        return ComponentMapIteratorSentinelEnd{};
    }
};

template <typename V, typename Alloc = mem::default_allocator<std::pair<V, bool>>>
class ComponentMap {
    mem::vector<std::pair<V, bool>, Alloc> map{};

    auto& at(const TypeUUID key) {
        if (key.id() >= map.size()) [[unlikely]] {
            map.reserve(key.id() + 8);
            map.fill_remaining();
        }
        return map[key.id()];
    }
public:
    ComponentMap() = default;

    template <typename... AArgs>
    ComponentMap(AArgs&&... args) : map(std::forward<AArgs>(args)...) {}

    auto& getUnchecked(const TypeUUID key) {
        return map[key.id()].first;
    }
    
    auto& operator [] (const TypeUUID key) { 
        return at(key).first;
    }

    V* find(const TypeUUID key) {
        auto& pos = at(key);
        if (!pos.second) return nullptr;
        return &pos.first;
    }

    const V* find(const TypeUUID key) const {
        if (key.id() >= map.size()) return nullptr;
        return map[key.id()].second ? &map[key.id()].first : nullptr;
    }

    template <typename... Args>
    V& emplace(const TypeUUID key, Args&&... args) {
        auto& pos = at(key);
        pos.second = true;
        pos.first = V(std::forward<Args>(args)...);
        return pos.first;
    }
    
    template <typename ...Args>
    V& getOrCreate(const TypeUUID key, Args&&... args) {
        auto& pos = at(key);

        if (pos.second)
            return pos.first;

        pos.second = true;
        pos.first = V(std::forward<Args>(args)...);
        return pos.first;
    }

    template <typename EmplaceFn>
    V& getOrEmplace(const TypeUUID key, EmplaceFn&& emplace) {
        auto& pos = at(key);

        if (pos.second)
            return pos.first;

        pos.second = true;
        pos.first = std::forward<EmplaceFn>(emplace)();
        return pos.first;
    }

    auto iterator(ComponentKind kind) {
        return ComponentMapIteratable<V, true>{
            kind, map.data(), (int)map.size()
        };
    }

    auto begin() {
        return ComponentMapIterator<V, false>{
            {}, map.data(), 0, (int)map.size()
        };
    }

    static auto end() {
        return ComponentMapIteratorSentinelEnd{};
    }

    void reserve(const size_t count) {
        map.reserve(count);
        map.fill_remaining();
    }
};

#include <ECS/utils.h>