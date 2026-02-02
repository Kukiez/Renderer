#pragma once

#include <array>
#include <iostream>
#include <AABB.h>
#include <memory>
#include <vector>

template <typename T>
class QuadTree {
    std::vector<T> objects;
    std::array<
        std::unique_ptr<QuadTree>, 4> children;
    AABB bounds;

    unsigned depth = 0;

    constexpr static unsigned maxObjects = 8;
    constexpr static unsigned maxDepth = 8;
public:
    explicit QuadTree(const AABB &bounds) : bounds(bounds) {}
    QuadTree(const glm::vec2 min, const glm::vec2 max) {
        bounds.center = glm::vec3((min + max) / 2.0f, 0.0f);
        bounds.halfSize = glm::vec3((max - min) / 2.0f, 0.0f);
        objects.reserve(maxObjects);
    }

    QuadTree(const AABB &bounds, const unsigned depth) : bounds(bounds), depth(depth) {
        objects.reserve(maxObjects);
    }

    void subdivide() {
        const glm::vec3 quarter = bounds.halfSize / 2.0f;

        for (int i = 0; i < 4; ++i) {
            glm::vec3 offset(
                (i & 1 ? 0.5f : -0.5f),
                (i & 2 ? 0.5f : -0.5f),
                0
            );

            AABB childBound = {
                bounds.center + offset * quarter * 2.0f,
                quarter
            };

            children[i] = std::make_unique<QuadTree>(childBound, depth + 1);
        }
    }

    std::vector<T*> query(glm::vec3 point) {
        std::vector<T*> result{};

        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (it->aabb.contains(point)) {
                result.push_back(&*it);
            }
        }

        if (children[0]) {
            for (auto& child : children) {
                auto res = child->query(point);
                result.insert(result.end(), res.begin(), res.end());
            }
        }
        return result;
    }

    void insert(const T& item) {
        if (children[0]) {
            bool inserted = false;

            for (const auto& child : children) {
                if (child->bounds.intersects(item.aabb)) {
                    child->insert(item);
                    inserted = true;
                    break;
                }
            }
            if (!inserted) {
                objects.push_back(item);
            }
        } else {
            objects.push_back(item);

            if (objects.size() > maxObjects && depth < maxDepth) {
                subdivide();

                auto currentObjects = std::move(objects);
                objects.clear();
                for (const auto& obj : currentObjects) {
                    insert(obj);
                }
            }
        }
    }

    bool remove(const T& item) {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (*it == item) {
                objects.erase(it);
                return true;
            }
        }
        if (!children[0]) return false;

        for (auto& child : children) {
            if (bool erased = child->remove(item)) return true;
        }
        return false;
    }
};
