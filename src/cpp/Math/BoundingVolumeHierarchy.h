#pragma once
#include "Shapes/Triangle.h"
#include "Shapes/AABB.h"
#include "Shapes/geom.h"
#include <algorithm>
#include <stack>
#include <memory/Span.h>
#include "Shapes/Ray.h"
#include <memory/vector.h>
#include <memory/byte_arena.h>

template <typename Primitive>
struct BVHRayResult : public RayResult {
    const Primitive* primitive;
};

template <typename T>
struct BVH_Primitive_Type {
    using primitive_type = std::conditional_t<
        std::constructible_from<AABB, T>, AABB,
        std::conditional_t<
            std::constructible_from<Triangle, T>, Triangle,
            void
        >
    >;
};

template <typename T>
using primitive_type_t = BVH_Primitive_Type<T>::primitive_type;

template <typename Primitive> requires (!std::is_same_v<primitive_type_t<Primitive>, void>)
// Primitive requires an operator const Geom&() where Geom: AABB or Triangle
class BoundingVolumeHierarchy {
public:
    using const_ref_primitive_type_t = const primitive_type_t<Primitive>&;
    using primitive_arena = mem::byte_arena<mem::same_alloc_schema, alignof(Primitive)>;
    using primitive_vector = mem::vector<Primitive, mem::byte_arena_adaptor<Primitive, primitive_arena>>;

    static const_ref_primitive_type_t primitive_cast(const Primitive& primitive) {
        return static_cast<const_ref_primitive_type_t>(primitive);
    }

    struct BVHNode {
        AABB bounds;
        BVHNode* parent = nullptr;
        BVHNode* left = nullptr;
        BVHNode* right = nullptr;
        primitive_vector primitives;

        BVHNode() = default;
        BVHNode(auto& arena) : primitives(&arena) {}

        bool isLeaf() const {
            return !left;
        }
    };
    template <typename CB>
    bool forEachIntersects(const AABB& aabb, CB&& cb, const BVHNode* node) const {
        if (!geom::intersects(aabb, node->bounds)) return false;

        if (node->isLeaf()) {
            for (const auto& primitive : node->primitives) {
                if (geom::intersects(aabb, primitive)) {
                    if (cb(primitive)) return true;
                }
            }
        } else {
            if (forEachIntersects(aabb, std::forward<CB>(cb), node->left)) return true;
            if (forEachIntersects(aabb, std::forward<CB>(cb), node->right)) return true;
        }
    }

    template <bool Transform, typename Callable>
    bool forEachRayIntersects(const Ray& ray, const glm::mat4& bvhWorldTransform,
        const glm::mat3& normalMatrix, Callable&& callable, const BVHNode* node) const
    {
        if (RayResult hit = ray.intersects(node->bounds); !hit.hasHit()) return false;

        if (node->isLeaf()) {
            for (const auto& primitive : node->primitives) {
                if (RayResult result = ray.intersects(primitive_cast(primitive)); result.hasHit()) {
                    BVHRayResult<Primitive> bvhResult;
                    if constexpr (Transform) {
                        glm::vec3 worldHitPos = glm::vec3(bvhWorldTransform * glm::vec4(result.hitPos, 1.0f));
                        glm::vec3 worldNormal = glm::normalize(normalMatrix * result.normal);

                        bvhResult.distance = result.distance;
                        bvhResult.hitPos = worldHitPos;
                        bvhResult.normal = worldNormal;
                        bvhResult.primitive = &primitive;
                    } else {
                        bvhResult.distance = result.distance;
                        bvhResult.hitPos = result.hitPos;
                        bvhResult.normal = result.normal;
                        bvhResult.primitive = &primitive;
                    }
                    if (callable(bvhResult)) return true;
                }
            }
        } else {
            if (forEachRayIntersects<Transform>(ray, bvhWorldTransform, normalMatrix, callable, node->left)) return true;
            return forEachRayIntersects<Transform>(ray, bvhWorldTransform, normalMatrix, callable, node->right);
        }
    }

    AABB computeCentroidBounds(Primitive* primitives, const size_t start, const size_t end) {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);

        for (auto i = start; i < end; ++i) {
            auto& primitive = primitives[i];
            glm::vec3 centroid = geom::centroid(primitive_cast(primitive));

            if (centroid.x < min.x) min.x = centroid.x;
            if (centroid.y < min.y) min.y = centroid.y;
            if (centroid.z < min.z) min.z = centroid.z;

            if (centroid.x > max.x) max.x = centroid.x;
            if (centroid.y > max.y) max.y = centroid.y;
            if (centroid.z > max.z) max.z = centroid.z;
        }
        return AABB::fromTo(min, max);
    }
private:
    static size_t reserveForNodes(const size_t forNodes, const size_t maxLeafSize) {
        return 3 * static_cast<size_t>(ceil(forNodes / maxLeafSize + 1)) + 1;
    }

    BVHNode* build(Primitive* primitives, const size_t start, const size_t end, const size_t maxLeafSize, BVHNode* parent = nullptr) {
        BVHNode* node = &nodes.emplace_back();
        node->bounds = primitives[start];
        node->parent = parent;

        for (auto i = start + 1; i < end; ++i) {
            node->bounds = geom::merge(node->bounds, primitives[i]);
        }

        size_t count = end - start;
        if (count <= maxLeafSize) {
            node->primitives = mem::make_vector_like<primitive_vector>(
                primitives + start, count, count,
                mem::make_byte_arena_adaptor<Primitive>(primitivesArena)
            );
            return node;
        }

        AABB centroidBounds = computeCentroidBounds(primitives, start, end);
        glm::vec3 extent = centroidBounds.max() - centroidBounds.min();

        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;

        constexpr int bucketCount = 12;

        struct BucketInfo {
            int count = 0;
            AABB bounds;
        };
        BucketInfo buckets[bucketCount];

        for (size_t i = start; i < end; ++i) {
            float centroidAxis = geom::centroid(primitives[i])[axis];
            int b = static_cast<int>(bucketCount * (centroidAxis - centroidBounds.min()[axis]) / (extent[axis] + 1e-5f));
            if (b == bucketCount) b = bucketCount - 1;
            ++buckets[b].count;
            buckets[b].bounds = geom::merge(buckets[b].bounds, primitives[i]);
        }
        float minCost = std::numeric_limits<float>::max();
        int minCostSplit = -1;

        for (int i = 1; i < bucketCount; ++i) {
            AABB b0, b1;
            int count0 = 0, count1 = 0;

            for (int j = 0; j < i; ++j) {
                b0 = geom::merge(b0, buckets[j].bounds);
                count0 += buckets[j].count;
            }
            for (int j = i; j < bucketCount; ++j) {
                b1 = geom::merge(b1, buckets[j].bounds);
                count1 += buckets[j].count;
            }

            float cost = 1 + (count0 * geom::surface_area(b0) + count1 * geom::surface_area(b1)) / geom::surface_area(node->bounds);
            if (cost < minCost) {
                minCost = cost;
                minCostSplit = i;
            }
        }
        if (minCost >= count) {
            node->primitives = mem::make_vector_like<primitive_vector>(
                primitives + start, count, count, /* this was the end-start */
                mem::make_byte_arena_adaptor<Primitive>(primitivesArena)
            );
            return node;
        }

        auto midIter = std::partition(primitives + start, primitives + end,
            [&](const Primitive& p) {
                const float centroidAxis = geom::centroid(primitive_cast(p))[axis];
                int b = static_cast<int>(bucketCount * (centroidAxis - centroidBounds.min()[axis]) / (extent[axis] + 1e-5f));
                if (b == bucketCount) b = bucketCount - 1;
                return b < minCostSplit;
            });

        size_t mid = midIter - primitives;

        node->left = build(primitives, start, mid, maxLeafSize, node);
        node->right = build(primitives, mid, end, maxLeafSize, node);
        return node;
    }

    static void replaceChild(BVHNode* parent, BVHNode* oldChild, BVHNode* newChild) {
        if (parent->left == oldChild) {
            parent->left = newChild;
        } else if (parent->right == oldChild) {
            parent->right = newChild;
        }
        newChild->parent = parent;
        parent->bounds = geom::merge(parent->left->bounds, parent->right->bounds);
    }

    BVHNode* findBestSibling(const AABB& newBounds, BVHNode* current) {
        BVHNode* best = nullptr;
        float bestCost = std::numeric_limits<float>::infinity();

        std::stack<BVHNode*> stack;
        stack.push(current);

        while (!stack.empty()) {
            BVHNode* node = stack.top();
            stack.pop();
            AABB combined = geom::merge(node->bounds, newBounds);
            float cost = geom::surface_area(combined);

            if (node->isLeaf()) {
                if (cost < bestCost) {
                    best = node;
                    bestCost = cost;
                }
            } else {
                stack.push(node->left);
                stack.push(node->right);
            }
        }
        return best;
    }

    mem::vector<BVHNode> nodes;
    size_t primitiveCount = 0;
    primitive_arena primitivesArena;
    BVHNode* _root = nullptr;
public:
    BoundingVolumeHierarchy() = default;

    template <typename Range>
    BoundingVolumeHierarchy(Range&& range, const size_t maxLeafSize) {
        build(range, maxLeafSize);
    }

    BoundingVolumeHierarchy(mem::range<Primitive> primitives, const size_t maxLeafSize) {
        build(primitives, maxLeafSize);
    }

    template <typename Range>
    void build(Range&& primitives, const size_t maxLeafSize) {
        if (_root) {
            clear();
        }
        if (!primitivesArena.has_for(mem::type_info_of<Primitive>, primitives.size())) {
            primitivesArena.initialize(mem::type_info_of<Primitive>, primitives.size());
        }

        nodes = reserveForNodes(primitives.size(), maxLeafSize);

        auto ptr = primitivesArena.allocate<Primitive>(primitives.size());

        if constexpr (std::is_trivially_copyable_v<Primitive>) {
            memcpy(ptr, primitives.data(), sizeof(Primitive) * primitives.size());
        } else {
            static_assert(false, "Not supported yet or never");
        }
        _root = build(ptr, 0, primitives.size(), maxLeafSize);
    }

    BoundingVolumeHierarchy rebuild(const size_t maxLeafSize, const size_t extraPrimitives = 0) {
        const size_t size = this->primitiveCount;
        const size_t capacity = size + extraPrimitives;
        BoundingVolumeHierarchy bvh;

        bvh.primitivesArena.initialize(mem::type_info_of<Primitive>, capacity);
        bvh.nodes.reserve(reserveForNodes(capacity, maxLeafSize));
        auto ptr = bvh.primitivesArena.template allocate_or_fail<Primitive>(size);

        cexpr::require(ptr);

        if constexpr (std::is_trivially_copyable_v<Primitive>) {
            primitivesArena.copy_all_memory_to(ptr, mem::type_info_of<Primitive>);
        } else {
            static_assert(false, "Not supported yet or never");
        }
        bvh._root = bvh.build(ptr, 0, size, maxLeafSize);
        bvh.primitiveCount = size;
        return std::move(bvh);
    }

    BoundingVolumeHierarchy rebuildWith(mem::range<Primitive> primitives, const size_t maxLeafSize) {
        const size_t size = primitives.size() + primitiveCount;

        BoundingVolumeHierarchy bvh;
        bvh.primitivesArena.initialize(mem::type_info_of<Primitive>, size);
        auto ptr = bvh.primitivesArena.template allocate_or_fail<Primitive>(size);

        cexpr::require(ptr);

        if constexpr (std::is_trivially_copyable_v<Primitive>) {
            void* remaining = primitivesArena.copy_all_memory_to(ptr, mem::type_info_of<Primitive>);
            mem::copy(mem::type_info_of<Primitive>, remaining, primitives.data(), primitives.size());
        } else {
            static_assert(false, "Not supported yet or never");
        }
        bvh._root = bvh.build(ptr, 0, size, maxLeafSize);
        bvh.primitiveCount = size;
        return std::move(bvh);
    }

    void insert(const Primitive& primitive) {
        if (nodes.size() + 2 >= nodes.capacity()) {
            const size_t newCapacity = std::max(nodes.capacity() * 2, 16ull);
            nodes.reserve(newCapacity);
            primitivesArena.reserve(newCapacity * sizeof(Primitive));
        }
        insert(_root, primitive, primitive);
        ++primitiveCount;
    }

    void insert(BVHNode*& root, const AABB& newBounds, const Primitive& payload) {
        BVHNode* leaf = &nodes.emplace_back(primitivesArena);
        leaf->bounds = newBounds;

        leaf->primitives.emplace_back(payload);

        if (!root) {
            root = leaf;
            return;
        }

        BVHNode* sibling = findBestSibling(newBounds, root);
        BVHNode* newParent = &nodes.emplace_back(primitivesArena);
        newParent->bounds = geom::merge(sibling->bounds, newBounds);
  
        newParent->left = sibling;
        newParent->right = leaf;
              
        BVHNode* oldParent = sibling->parent;

        if (oldParent) {
            replaceChild(oldParent, sibling, newParent);
            newParent->parent = oldParent;
        } else {
            root = newParent;
            newParent->parent = nullptr;
        }
        sibling->parent = newParent;
        leaf->parent = newParent;
        refitUpwards(newParent);
    }

    void refitUpwards(BVHNode* node) {
        while (node) {
            if (!node->isLeaf()) {
                node->bounds = geom::merge(node->left->bounds, node->right->bounds);
            }
            node = node->parent;
        }
    }

    void updateBounds(BVHNode* node) {
        AABB newBounds{};
        for (const auto& prim : node->primitives) {
            newBounds = geom::merge(newBounds, prim);
        }
        node->bounds = newBounds;
    }

    struct FindResult {
        BVHNode* node = nullptr;
        Primitive* primitive = nullptr;

        FindResult() = default;
        FindResult(BVHNode* node, Primitive* primitive) : node(node), primitive(primitive) {}

        operator bool() const {
            return node;
        }
    };

    template <typename T>
    FindResult find(const T& item) {
        for (auto& node : nodes) {
            for (auto& prim : node.primitives) {
                if (prim == item) {
                    return FindResult(&node, &prim);
                }                
            }
        }
        return {};
    }

    template <typename Callable>
    /* bool(const Primitive&) -> return true to early exit */
    requires std::is_invocable_r_v<bool, Callable, const Primitive&>
    void forEachIntersects(const AABB& aabb, Callable&& callable) const {
        if (!_root) return;
        forEachIntersects(aabb, std::forward<Callable>(callable), _root);
    }

    template <typename Callable>
    /* bool(const Primitive&) -> return true to early exit*/
    void forEachRayIntersects(const Ray& ray, const glm::mat4& bvhWorldTransform, Callable&& callable) const {
        if (!_root) return;
        glm::mat4 bvhLocalTransform = glm::inverse(bvhWorldTransform);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(bvhWorldTransform)));
        Ray localRay = geom::transform(ray, bvhLocalTransform);

        forEachRayIntersects<true>(localRay, bvhWorldTransform, normalMatrix, std::forward<Callable>(callable), _root);
    }

    template <typename Callable>
    void forEachRayIntersects(const Ray& ray, Callable&& callable) const {
        if (!_root) return;
        static glm::mat4 identity{};
        forEachRayIntersects<false>(ray, identity, identity, std::forward<Callable>(callable), _root);
    }

    template <typename Callable>
    /**
     * @param localRay should be in BVH-Local space
     * @param callable bool(const Primitive&) -> return true to early exit
     */
    void forEachRayIntersects(const Ray& localRay, const glm::mat4& bvhWorldTransform, const glm::mat3& normalMatrix, Callable&& callable) const {
        if (!_root) return;
        forEachRayIntersects<true>(localRay, bvhWorldTransform, normalMatrix, std::forward<Callable>(callable), _root);
    }

    template <typename Callable>
    /* void(const BVHNode& node)*/
    requires std::is_invocable_r_v<void, Callable, const BVHNode&>
    void forEachNode(Callable&& callable) const {
        for (const auto& node : nodes) {
            callable(node);
        }
    }

    BVHNode* root() {
        return _root;
    }

    const BVHNode* root() const {
        return _root;
    }

    const AABB& bounds() const {
        return root()->bounds;
    }

    size_t nodeCount() const {
        return nodes.count();
    }

    size_t size() const {
        return primitivesArena.bytes_used() / sizeof(Primitive);
    }

    bool empty() const {
        return !size();
    }

    void clear() {
        nodes.clear();
        primitivesArena.reset_shrink();
        _root = nullptr;
    }
};

template <typename P>
using BVH = BoundingVolumeHierarchy<P>;