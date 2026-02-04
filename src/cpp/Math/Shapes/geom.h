#pragma once
#include "Capsule.h"
#include "OOBB.h"
#include "AABB.h"
#include "Ray.h"
#include "../MathAPI.h"

namespace geom {
    struct MTVResult {
        glm::vec3 direction;
        float depth;
    };

    MATHAPI OOBB toOOBB(const Capsule& cap);
    MATHAPI OOBB toOOBB(const AABB& aabb);

    MATHAPI AABB toAABB(const OOBB& oobb);
    MATHAPI AABB toAABB(const Capsule& cap);

    MATHAPI bool intersects(const AABB& aabb1, const AABB& aabb2);
    MATHAPI bool intersects(const OOBB& oobb1, const OOBB& oobb2);
    MATHAPI bool intersects(const Capsule& cap1, const Capsule& cap2);

    /* tests if oobb1 contains oobb2 */
    MATHAPI bool contains(const OOBB& oobb1, const OOBB& oobb2);
    MATHAPI bool contains(const AABB& aabb1, const AABB& aabb2);

    MATHAPI float projectExtent(const OOBB& oobb, glm::vec3 extent);

    MATHAPI MTVResult mtv(const OOBB& oobb1, const OOBB& oobb2);
    MATHAPI MTVResult mtv(const AABB& aabb1, const AABB& aabb2);

    MATHAPI AABB expand(const AABB& aabb, glm::vec3 halfSize);

    MATHAPI glm::vec3 min(const AABB& aabb);
    MATHAPI glm::vec3 max(const AABB& aabb);

    MATHAPI AABB aabb_cast(const OOBB& oobb);
    MATHAPI OOBB oobb_cast(const AABB& aabb);

    MATHAPI AABB merge(const AABB& aabb1, const AABB& aabb2);

    MATHAPI glm::vec3 centroid(const AABB& aabb1);

    MATHAPI glm::vec3 extent(const AABB& aabb);
    MATHAPI float surface_area(const AABB& aabb);

    MATHAPI AABB inverse_transform(const AABB& worldAABB, const glm::mat4& worldTransform);

    struct MATHAPI transformer {
        AABB operator () (const AABB& aabb, const glm::mat4& worldTransform) const;
        AABB operator () (const AABB& aabb, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.f)) const;
        Ray operator () (const Ray& ray, const glm::mat4& worldTransform) const;

        OOBB operator () (const OOBB& oobb, const glm::mat4& worldTransform) const;
    };

    constexpr static transformer transform{};

    struct hash {
        static size_t hash_float(const float f) {
            uint32_t bits;
            std::memcpy(&bits, &f, sizeof(float));
            return std::hash<uint32_t>{}(bits);
        }

        static size_t vec3_hash(const glm::vec3 v) {
            size_t h = 0;
            auto hash_combine = [&h](size_t val) {
                h ^= val + 0x9e3779b9 + (h << 6) + (h >> 2);
            };
            hash_combine(hash_float(v.x));
            hash_combine(hash_float(v.y));
            hash_combine(hash_float(v.z));
            return h;
        }

        size_t operator() (const AABB& aabb) const noexcept {
            size_t h = 0;
            auto hash_combine = [&h](size_t val) {
                h ^= val + 0x9e3779b9 + (h << 6) + (h >> 2);
            };
            hash_combine(vec3_hash(aabb.center));
            hash_combine(vec3_hash(aabb.halfSize));
            return h;
        }
    };

    inline int floorDiv(int a, int b) {
        return (a >= 0) ? (a / b) : ((a - b + 1) / b);
    }

    inline glm::ivec3 floorDiv3(glm::vec3 value, const glm::ivec3 divisor) {
        return {
            floorDiv(static_cast<int>(glm::floor(value.x)), divisor.x),
            floorDiv(static_cast<int>(glm::floor(value.y)), divisor.y),
            floorDiv(static_cast<int>(glm::floor(value.z)), divisor.z
        )};
    }

    template <std::integral I>
    glm::ivec3 floorDiv3(glm::vec3 value, I divisor) {
        return floorDiv3(value, glm::ivec3(static_cast<int>(divisor)));
    }
}
