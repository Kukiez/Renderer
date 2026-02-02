#pragma once

#include "AABB.h"
#include <memory>
#include <array>

#include "Ray.h"

template <typename T>
class Octree {
    bool hasChildren() {
        return children[0] != nullptr;
    }

    void recursiveSubdivide() {
        subdivide();
        if (depth < maxDepth) {
            for (const auto& child : children) {
                child->subdivide();
            }            
        }
    }
public:
    AABB bounds;
    int depth = 0;
    int maxDepth = 0;
    int size = 0;

    std::array<
        std::unique_ptr<Octree>, 8
    > children = { nullptr };

    std::vector<T> objects;

    Octree() = default;
    
    Octree(const AABB &bounds, const int depth, const int size, const int maxDepth) : bounds(bounds), depth(depth), maxDepth(maxDepth), size(size) {
        objects.reserve(size);
    }

    explicit Octree(const AABB &bounds, const int size, const int maxDepth, const bool subdivideFromStart = false) : bounds(bounds), maxDepth(maxDepth), size(size) {
        objects.reserve(size);

        if (subdivideFromStart) {
            recursiveSubdivide();
        }
    } 

    void subdivide() {
        if (children[0]) return;

        const glm::vec3 quarter = bounds.halfSize / 2.0f;

        for (int i = 0; i < 8; ++i) {
            glm::vec3 offset( 
                (i & 1 ? 0.5f : -0.5f),
                (i & 2 ? 0.5f : -0.5f),
                (i & 4 ? 0.5f : -0.5f)
            );

            AABB childBound = {
                bounds.center + offset * quarter * 2.0f,
                quarter
            };
            children[i] = std::make_unique<Octree>(childBound, depth + 1, size, maxDepth);
        }
    }

    void insert(const T& item, Octree* parent = nullptr) {
        if (children[0]) {
            bool inserted = false;
    
            for (const auto& child : children) {
                if (child->bounds.contains(item.aabb)) {
                    child->insert(item, this);
                    inserted = true;
                    break;
                }
            }
    
            if (!inserted) {
                objects.push_back(item);
            }
        } else {
            objects.push_back(item);

            if (objects.size() > size && depth < maxDepth) {
                subdivide();
    
                auto currentObjects = std::move(objects);
                objects.clear();
                for (const auto& obj : currentObjects) {
                    insert(obj, this);
                }
            }
        }
    }

    bool remove(const T& item) {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (*it == item) {
                it = objects.erase(it);
                return true;
            }
        }
        if (children[0]) {
            for (const auto& child : children) {
                if (child->remove(item)) return true;
            }
        }
        return false;
    }

    template <typename Range>
    void fill(Range begin, Range end) {
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    std::vector<T*> allThatFit(AABB aabb) {
        std::vector<T*> result{};

        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (it->aabb.contains(aabb)) {
                result.push_back(&*it);
            }
        }

        if (children[0]) {
            for (auto& child : children) {
                auto res = child->allThatFit(aabb);
                result.insert(result.end(), res.begin(), res.end());
            }
        }
        return result;
    }

    void _AllThatIntersects(const AABB& aabb, std::vector<T*>& result) {
        if (!bounds.intersects(aabb)) return;

        if (!objects.empty())
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (it->aabb.intersects(aabb)) {
                result.push_back(&*it);
            }
        }
        if (children[0]) {
            for (auto& child : children) {
                child->_AllThatIntersects(aabb, result);
            }
        }
    }

    std::vector<T*> allThatIntersect(const AABB &aabb) {
        std::vector<T*> result{};
        _AllThatIntersects(aabb, result);
        return result;
    }

    void allThatIntersect(const AABB& aabb, std::vector<T*>& result) {
        _AllThatIntersects(aabb, result);
    }

    template <typename Callable>
    requires std::is_invocable_r_v<void, Callable, T&>
    void forEach(Callable&& callable) {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            callable(*it);
        }

        if (children[0]) {
            for (auto& child : children) {
                child->forEach(std::forward<Callable>(callable));
            }
        }
    }

    template <typename Callable>
    // return true -> skips children
    // return false -> continues
    void onEachNode(Callable&& callable) {
        if constexpr (std::is_invocable_r_v<bool, Callable, Octree<T>>) {
            if (callable(*this)) return;            
        } else {
            callable(*this);
        }

        if (children[0]) {
            for (auto& child : children) {
                child->onEachNode(std::forward<Callable>(callable));
            }
        }
    }

    std::vector<std::pair<RayResult, T*>> raycast(Ray ray) {
        std::vector<std::pair<RayResult, T*>> result{};

        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (auto r = ray.intersects(it->aabb); r.hasHit()) {
                result.emplace_back(r, &*it);
            }
        }

        if (children[0]) {
            for (auto& child : children) {
                auto res = child->raycast(ray);
                result.insert(result.end(), res.begin(), res.end());
            }
        }
        return result;
    }

    bool empty() const {
        return objects.empty();
    }
};