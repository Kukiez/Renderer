#pragma once
#include <glm/glm.hpp>
#include <Math/Shapes/AABB.h>

enum class FrustumPlane {
    LEFT, RIGHT,
    BOTTOM, TOP,
    NEAR_PLANE, FAR_PLANE
};

struct Frustum {
    glm::vec4 planes[6];
    glm::mat4 view;
    glm::mat4 projection;

    void extract(const glm::mat4& m) {
        const glm::mat4 vpt = glm::transpose(m);

        planes[0] = vpt[3] + vpt[0];
        planes[1] = vpt[3] - vpt[0];
        planes[2] = vpt[3] + vpt[1];
        planes[3] = vpt[3] - vpt[1];

        planes[4] = vpt[3] + vpt[2];
        planes[5] = vpt[3] - vpt[2];

        for (int i = 0; i < 6; ++i) {
            const float len = glm::length(glm::vec3(planes[i]));
            planes[i] /= len;
        }
    }

    explicit Frustum(const glm::mat4& view, const glm::mat4& proj) : view(view), projection(proj) {
        extract(proj * view);
    }

    static bool isInsideViewDistance(const glm::vec3& origin, const AABB& box, const float viewDistance) {
        const glm::vec3 clampedPoint = glm::clamp(origin, box.min(), box.max());
        const float d = glm::length(origin - clampedPoint);

        return d <= viewDistance;
    }

    static bool isFullyInsideViewDistance(const glm::vec3& cameraPos, const AABB& box, float viewDistance) {
        glm::vec3 corners[8] = {
            { box.min().x, box.min().y, box.min().z },
            { box.min().x, box.min().y, box.max().z },
            { box.min().x, box.max().y, box.min().z },
            { box.min().x, box.max().y, box.max().z },
            { box.max().x, box.min().y, box.min().z },
            { box.max().x, box.min().y, box.max().z },
            { box.max().x, box.max().y, box.min().z },
            { box.max().x, box.max().y, box.max().z },
        };
        for (const auto& corner : corners) {
            float dist = glm::distance(cameraPos, corner);
            if (dist > viewDistance) {
                return false;
            }
        }
        return true;
    }


    static std::array<glm::vec3, 8> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
        const auto inv = glm::inverse(proj * view);

        std::array<glm::vec3, 8> frustumCorners;
        int i= 0;

        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    const glm::vec4 pt =
                        inv * glm::vec4(
                            2.0f * x - 1.0f,
                            2.0f * y - 1.0f,
                            2.0f * z - 1.0f,
                            1.0f);
                    frustumCorners[i++] = glm::vec3(pt / pt.w);
                }
            }
        }
        return frustumCorners;
    }

    bool intersectsPlane(const AABB& box, const FrustumPlane plane) const {
        const glm::vec4 normal = planes[static_cast<int>(plane)];

        const float r =
            box.halfSize.x * std::abs(normal.x) +
            box.halfSize.y * std::abs(normal.y) +
            box.halfSize.z * std::abs(normal.z);
        const float s = glm::dot(glm::vec3(normal), box.center) + normal.w;

        return std::abs(s) <= r;
    }

    bool intersectsAnyPlane(const AABB& box) const {
        for (int i = 0; i < 6; ++i) {
            if (intersectsPlane(box, FrustumPlane{i})) {
                return true;
            }
        }
        return false;
    }

    bool isAABBInsideFrustum(const AABB& box) const {
        for (int i = 0; i < 6; ++i) {
            auto& plane = planes[i];

            glm::vec3 axisVert;

            axisVert.x = plane.x < 0.0f ? box.min().x : box.max().x;
            axisVert.y = plane.y < 0.0f ? box.min().y : box.max().y;
            axisVert.z = plane.z < 0.0f ? box.min().z : box.max().z;

            if (dot(glm::vec3(plane), axisVert) + plane.w < 0.0f) {
                return false;
            }
        }
        return true;
    }

    glm::vec3 intersectPlanes(FrustumPlane A,  FrustumPlane B, FrustumPlane C) const {
        const glm::vec4 p1 = planes[static_cast<int>(A)];
        const glm::vec4 p2 = planes[static_cast<int>(B)];
        const glm::vec4 p3 = planes[static_cast<int>(C)];

        const glm::vec3 n1(p1);
        const glm::vec3 n2(p2);
        const glm::vec3 n3(p3);

        const glm::vec3 cross23 = glm::cross(n2, n3);
        const glm::vec3 cross31 = glm::cross(n3, n1);
        const glm::vec3 cross12 = glm::cross(n1, n2);

        const float denom = glm::dot(n1, cross23);
        if (std::abs(denom) < 1e-6f)
            return glm::vec3(0.0f);

        return (-p1.w * cross23
                -p2.w * cross31
                -p3.w * cross12) / denom;
    }

    static AABB asAABB(const glm::mat4& projection, const glm::mat4& view) {
        glm::mat4 invViewProj = glm::inverse(projection * view);
        glm::vec3 frustumCorners[8];

        int index = 0;
        for (int x = 0; x <= 1; ++x) {
            for (int y = 0; y <= 1; ++y) {
                for (int z = 0; z <= 1; ++z) {
                    auto ndc = glm::vec4(
                        x * 2.0f - 1.0f, // -1 or +1
                        y * 2.0f - 1.0f,
                        z * 2.0f - 1.0f,
                        1.0f
                    );
                    glm::vec4 world = invViewProj * ndc;
                    frustumCorners[index++] = glm::vec3(world) / world.w;
                }
            }
        }
        auto minCorner = glm::vec3(std::numeric_limits<float>::max());
        auto maxCorner = glm::vec3(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; ++i) {
            minCorner = glm::min(minCorner, frustumCorners[i]);
            maxCorner = glm::max(maxCorner, frustumCorners[i]);
        }
        AABB frustumAABB = AABB::fromTo( minCorner, maxCorner );
        return  frustumAABB;
    }

    static AABB toAABB(const glm::mat4 &invProjView) {
        std::array<glm::vec3, 8> frustumCorners;

        std::array<glm::vec4, 8> ndcCorners = {
            glm::vec4(-1, -1, -1, 1), glm::vec4(1, -1, -1, 1),
            glm::vec4(1,  1, -1, 1), glm::vec4(-1, 1, -1, 1),
            glm::vec4(-1, -1,  1, 1), glm::vec4(1, -1,  1, 1),
            glm::vec4(1,  1,  1, 1), glm::vec4(-1, 1,  1, 1),
        };

        for (int i = 0; i < 8; ++i) {
            glm::vec4 world = invProjView * ndcCorners[i];
            frustumCorners[i] = glm::vec3(world) / world.w;
        }

        glm::vec3 minCorner = frustumCorners[0];
        glm::vec3 maxCorner = frustumCorners[0];

        for (int i = 1; i < 8; ++i) {
            minCorner = glm::min(minCorner, frustumCorners[i]);
            maxCorner = glm::max(maxCorner, frustumCorners[i]);
        }
        AABB frustumAABB = AABB::fromTo(minCorner, maxCorner);
        return frustumAABB;
    }

    AABB asAABB() const {
        auto corners = getFrustumCornersWorldSpace(projection, view);

        glm::vec3 minCorner = corners[0];
        glm::vec3 maxCorner = corners[0];

        for (int i = 1; i < 8; ++i) {
            minCorner = glm::min(minCorner, corners[i]);
            maxCorner = glm::max(maxCorner, corners[i]);
        }

        return AABB::fromTo(minCorner, maxCorner);
        // glm::vec3 frustumCorners[8];
        // frustumCorners[0] = intersectPlanes(FrustumPlane::NEAR_PLANE, FrustumPlane::LEFT, FrustumPlane::TOP);
        // frustumCorners[1] = intersectPlanes(FrustumPlane::NEAR_PLANE, FrustumPlane::RIGHT, FrustumPlane::TOP);
        // frustumCorners[2] = intersectPlanes(FrustumPlane::NEAR_PLANE, FrustumPlane::LEFT, FrustumPlane::BOTTOM);
        // frustumCorners[3] = intersectPlanes(FrustumPlane::NEAR_PLANE, FrustumPlane::RIGHT, FrustumPlane::BOTTOM);
        // frustumCorners[4] = intersectPlanes(FrustumPlane::FAR_PLANE, FrustumPlane::LEFT, FrustumPlane::TOP);
        // frustumCorners[5] = intersectPlanes(FrustumPlane::FAR_PLANE, FrustumPlane::RIGHT, FrustumPlane::TOP);
        // frustumCorners[6] = intersectPlanes(FrustumPlane::FAR_PLANE, FrustumPlane::LEFT, FrustumPlane::BOTTOM);
        // frustumCorners[7] = intersectPlanes(FrustumPlane::FAR_PLANE, FrustumPlane::RIGHT, FrustumPlane::BOTTOM);
        //
        // auto minCorner = glm::vec3(std::numeric_limits<float>::max());
        // auto maxCorner = glm::vec3(std::numeric_limits<float>::lowest());
        //
        // for (int i = 0; i < 8; ++i) {
        //     minCorner = glm::min(minCorner, frustumCorners[i]);
        //     maxCorner = glm::max(maxCorner, frustumCorners[i]);
        // }
        // const AABB frustumAABB = { minCorner, maxCorner };
        // return  frustumAABB;
    }
};