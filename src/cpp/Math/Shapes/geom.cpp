#include "geom.h"

std::array<glm::vec3, 8> OOBB::getCorners() const {
    std::array<glm::vec3, 8> corners{};
    const glm::vec3 axes[] = {
        orientation[0] * halfSize.x,
        orientation[1] * halfSize.y,
        orientation[2] * halfSize.z
    };

    size_t i = 0;
    for (int x = -1; x <= 1; x += 2)
        for (int y = -1; y <= 1; y += 2)
            for (int z = -1; z <= 1; z += 2) {
                corners[i++] = center + static_cast<float>(x) * axes[0] + static_cast<float>(y) * axes[1] + static_cast<float>(z) * axes[2];
            }

    return corners;
}

OOBB geom::toOOBB(const Capsule &cap) {
    const glm::vec3 center = 0.5f * (cap.p1 + cap.p2);
    const glm::vec3 axis = glm::normalize(cap.p2 - cap.p1);

    const glm::vec3 up = axis;
    glm::vec3 right = glm::normalize(glm::cross(up, glm::vec3(0, 1, 0)));
    if (glm::length(right) < 0.001f)
        right = glm::normalize(glm::cross(up, glm::vec3(1, 0, 0)));
    const glm::vec3 forward = glm::cross(right, up);

    const auto orientation = glm::mat3(right, up, forward);
    const float halfLength = 0.5f * glm::length(cap.p2 - cap.p1);
    const glm::vec3 halfSize(cap.radius, halfLength + cap.radius, cap.radius);

    return OOBB{center, halfSize, orientation};
}

OOBB geom::toOOBB(const AABB &aabb) {
    return {aabb.center, aabb.halfSize, 0, 0, 0};
}

AABB geom::expand(const AABB& aabb, glm::vec3 halfSize) {
    auto min = aabb.min();
    auto max = aabb.max();

    min -= halfSize;
    max += halfSize;
    return AABB::fromTo(min, max);
}

AABB geom::toAABB(const OOBB &oobb) {
    const std::array<glm::vec3, 8> corners = oobb.getCorners();
    glm::vec3 min = corners[0];
    glm::vec3 max = corners[0];
    for (int i = 1; i < 8; ++i) {
        min = glm::min(min, corners[i]);
        max = glm::max(max, corners[i]);
    }
    return AABB{ min, max };
}

AABB geom::toAABB(const Capsule &cap) {
    const glm::vec3 minPoint = glm::min(cap.p1, cap.p2) - glm::vec3(cap.radius);
    const glm::vec3 maxPoint = glm::max(cap.p1, cap.p2) + glm::vec3(cap.radius);
    return AABB::fromTo(minPoint, maxPoint);
}

bool geom::intersects(const AABB &aabb1, const AABB &aabb2) {
    const glm::vec3 thisMin = aabb1.min();
    const glm::vec3 thisMax = aabb1.max();
    const glm::vec3 otherMin = aabb2.min();
    const glm::vec3 otherMax = aabb2.max();

    return !(otherMin.x > thisMax.x || otherMax.x < thisMin.x ||
             otherMin.y > thisMax.y || otherMax.y < thisMin.y ||
             otherMin.z > thisMax.z || otherMax.z < thisMin.z);
}

bool geom::intersects(const OOBB &oobb1, const OOBB &oobb2) {
    const glm::vec3 A_axes[3] = {
        oobb1.orientation[0],
        oobb1.orientation[1],
        oobb1.orientation[2]
    };
    const glm::vec3 B_axes[3] = {
        oobb2.orientation[0],
        oobb2.orientation[1],
        oobb2.orientation[2]
    };
    float R[3][3], AbsR[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            constexpr float EPSILON = 1e-6f;
            R[i][j] = glm::dot(A_axes[i], B_axes[j]);
            AbsR[i][j] = std::abs(R[i][j]) + EPSILON;
        }
    }
    glm::vec3 t = oobb2.center - oobb1.center;
    t = glm::vec3(glm::dot(t, A_axes[0]), glm::dot(t, A_axes[1]), glm::dot(t, A_axes[2]));

    for (int i = 0; i < 3; ++i) {
        const float ra = oobb1.halfSize[i];
        if (float rb = oobb2.halfSize[0] * AbsR[i][0] + oobb2.halfSize[1] * AbsR[i][1] + oobb2.halfSize[2] * AbsR[i][2];
            std::abs(t[i]) > ra + rb) return false;
    }
    for (int j = 0; j < 3; ++j) {
        float ra = oobb1.halfSize[0] * AbsR[0][j] + oobb1.halfSize[1] * AbsR[1][j] +oobb1. halfSize[2] * AbsR[2][j];
        float rb = oobb2.halfSize[j];
        if (float tDot = std::abs(t[0] * R[0][j] + t[1] * R[1][j] + t[2] * R[2][j]);
            tDot > ra + rb) return false;
    }
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            float ra = oobb1.halfSize[(i+1)%3] * AbsR[(i+2)%3][j] + oobb1.halfSize[(i+2)%3] * AbsR[(i+1)%3][j];
            float rb = oobb2.halfSize[(j+1)%3] * AbsR[i][(j+2)%3] + oobb2.halfSize[(j+2)%3] * AbsR[i][(j+1)%3];
            float tval = std::abs(t[(i+2)%3] * R[(i+1)%3][j] - t[(i+1)%3] * R[(i+2)%3][j]);
            if (tval > ra + rb) return false;
        }
    return true;
}

bool geom::intersects(const Capsule &cap1, const Capsule &cap2) {
    const glm::vec3 d1 = cap1.p2 - cap1.p1;
    const glm::vec3 d2 = cap2.p2 - cap2.p1;
    const glm::vec3 r = cap1.p1 - cap2.p1;
    const float a = glm::dot(d1, d1);
    const float e = glm::dot(d2, d2);
    const float f = glm::dot(d2, r);

    float sVal = 0.0f;
    float tVal = 0.0f;

    const float c = glm::dot(d1, r);
    const float b = glm::dot(d1, d2);

    if (const float denom = a * e - b * b; denom != 0.0f) {
        sVal = glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
    }
    tVal = (b * sVal + f) / e;

    if (tVal < 0.0f) {
        tVal = 0.0f;
        sVal = glm::clamp(-c / a, 0.0f, 1.0f);
    } else if (tVal > 1.0f) {
        tVal = 1.0f;
        sVal = glm::clamp((b - c) / a, 0.0f, 1.0f);
    }
    const glm::vec3 c1 = cap1.p1 + d1 * sVal;
    const glm::vec3 c2 = cap1.p2 + d2 * tVal;
    const float radiusSum = cap1.radius + cap2.radius;
    return glm::length2(c1 - c2) <= radiusSum * radiusSum;
}

bool geom::contains(const OOBB &oobb1, const OOBB &oobb2) {
    const glm::mat3 aRotationInv = glm::transpose(oobb1.orientation);

    for (glm::vec3 corner : oobb2.getCorners()) {
        const glm::vec3 local = aRotationInv * (corner - oobb1.center);

        if (glm::abs(local.x) > oobb1.halfSize.x ||
            glm::abs(local.y) > oobb1.halfSize.y ||
            glm::abs(local.z) > oobb1.halfSize.z)
            return false;
    }
    return true;
}

bool geom::contains(const AABB &aabb1, const AABB &aabb2) {
    const glm::vec3 thisMin = aabb1.min();
    const glm::vec3 thisMax = aabb1.max();
    const glm::vec3 otherMin = aabb2.min();
    const glm::vec3 otherMax = aabb2.max();

    return (otherMin.x >= thisMin.x && otherMax.x <= thisMax.x &&
            otherMin.y >= thisMin.y && otherMax.y <= thisMax.y &&
            otherMin.z >= thisMin.z && otherMax.z <= thisMax.z);
}

float geom::projectExtent(const OOBB &oobb, const glm::vec3 extent) {
    return
    std::abs(glm::dot(extent, oobb.orientation[0])) * oobb.halfSize.x +
    std::abs(glm::dot(extent, oobb.orientation[1])) * oobb.halfSize.y +
    std::abs(glm::dot(extent, oobb.orientation[2])) * oobb.halfSize.z;
}

geom::MTVResult geom::mtv(const OOBB &oobb1, const OOBB &oobb2) {
    glm::vec3 axes[15];
    int axisIndex = 0;

    for (int i = 0; i < 3; ++i) axes[axisIndex++] = oobb1.orientation[i];
    for (int i = 0; i < 3; ++i) axes[axisIndex++] = oobb2.orientation[i];

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            axes[axisIndex++] = glm::cross(oobb1.orientation[i], oobb2.orientation[j]);

    float minOverlap = std::numeric_limits<float>::max();
    auto mtvAxis = glm::vec3(0);

    const glm::vec3 delta = oobb2.center - oobb1.center;

    for (auto axe : axes) {
        glm::vec3 axis = glm::normalize(axe);
        if (glm::length2(axis) < 1e-6f) continue;

        const float aProj = projectExtent(oobb1, axis);
        const float bProj = projectExtent(oobb1, axis);
        const float dist = std::abs(glm::dot(delta, axis));
        const float overlap = aProj + bProj - dist;

        if (overlap <= 0.0f) return { glm::vec3(0), 0.0f };

        if (overlap < minOverlap) {
            minOverlap = overlap;
            mtvAxis = axis * (glm::dot(delta, axis) < 0 ? -1.0f : 1.0f);
        }
    }
    return { mtvAxis, minOverlap };
}

geom::MTVResult geom::mtv(const AABB &aabb1, const AABB &aabb2) {
    const glm::vec3 delta = aabb1.center - aabb2.center;
    const glm::vec3 overlap = (aabb1.halfSize + aabb2.halfSize) - glm::abs(delta);

    if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }
    glm::vec3 mtv(0.0f);

    if (overlap.x < overlap.y && overlap.x < overlap.z) {
        mtv.x = (delta.x < 0) ? -overlap.x : overlap.x;
    } else if (overlap.y < overlap.z) {
        mtv.y = (delta.y < 0) ? -overlap.y : overlap.y;
    } else {
        mtv.z = (delta.z < 0) ? -overlap.z : overlap.z;
    }
    const float depth = glm::min(overlap.x, glm::min(overlap.y, overlap.z));
    return {mtv, depth};
}

glm::vec3 geom::min(const AABB &aabb) {
    return aabb.center - aabb.halfSize;
}

glm::vec3 geom::max(const AABB &aabb) {
    return aabb.center + aabb.halfSize;
}



AABB geom::aabb_cast(const OOBB& oobb) {
    auto corners = oobb.getCorners();
    glm::vec3 min = corners[0];
    glm::vec3 max = corners[0];
    for (int i = 1; i < 8; ++i) {
        min = glm::min(min, corners[i]);
        max = glm::max(max, corners[i]);
    }
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 halfSize = (max - min) * 0.5f;

    return AABB{center, halfSize};
}

OOBB geom::oobb_cast(const AABB &aabb)
{
    return OOBB(aabb.center, aabb.halfSize, glm::quat(1, 0, 0, 0));
}

AABB geom::merge(const AABB &aabb1, const AABB &aabb2) {
    const glm::vec3 newMin = glm::min(aabb1.min(), aabb2.min());
    const glm::vec3 newMax = glm::max(aabb1.max(), aabb2.max());
    return AABB::fromTo(newMin, newMax);
}

glm::vec3 geom::centroid(const AABB &aabb1) {
    return aabb1.center;
}

glm::vec3 geom::extent(const AABB& aabb) {
    return aabb.max() - aabb.min();
}
float geom::surface_area(const AABB &aabb)
{
    glm::vec3 e = extent(aabb);
    return 2.0f * (e.x * e.y + e.y * e.z + e.x * e.z);
}

AABB geom::inverse_transform(const AABB &aabb, const glm::mat4 &transform) {
    glm::mat4 inv = glm::inverse(transform);
    auto corners = aabb.corners();

    glm::vec3 newMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 newMax = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& corner : corners) {
        glm::vec3 local = glm::vec3(inv * glm::vec4(corner, 1.0f));
        newMin = glm::min(newMin, local);
        newMax = glm::max(newMax, local);
    }
    return AABB::fromTo(newMin, newMax);
}

AABB geom::transformer::operator()(const AABB &aabb, const glm::mat4 &worldTransform) const
{
    auto corners = aabb.corners();

    glm::vec3 newMin(std::numeric_limits<float>::max());
    glm::vec3 newMax(std::numeric_limits<float>::lowest());

    for (const glm::vec3& corner : corners) {
        glm::vec3 transformed = glm::vec3(worldTransform * glm::vec4(corner, 1.0f));
        newMin = glm::min(newMin, transformed);
        newMax = glm::max(newMax, transformed);
    }

    return AABB::fromTo(newMin, newMax);
}

AABB geom::transformer::operator()(const AABB &aabb, const glm::vec3 &position, const glm::vec3 &scale) const
{
    return {aabb.center + position, aabb.halfSize * scale};
}

Ray geom::transformer::operator()(const Ray &ray, const glm::mat4 &worldTransform) const
{
    glm::vec3 newOrigin = glm::vec3(worldTransform * glm::vec4(ray.origin, 1.0f));

    glm::vec3 transformedDirection = glm::vec3(worldTransform * glm::vec4(ray.direction, 0.0f));
    float scaleFactor = glm::length(transformedDirection);
    glm::vec3 newDirection = glm::normalize(transformedDirection);

    return {
        newOrigin,
        newDirection,
        ray.length * scaleFactor
    };
}

OOBB geom::transformer::operator()(const OOBB &oobb, const glm::mat4 &worldTransform) const
{
    glm::vec3 newCenter = glm::vec3(worldTransform * glm::vec4(oobb.center, 1.0f));
    glm::mat3 linearPart = glm::mat3(worldTransform);

    glm::vec3 newAxes[3];
    for (int i = 0; i < 3; ++i) {
        newAxes[i] = linearPart * oobb.orientation[i];
    }
    glm::vec3 newHalfExtents;
    for (int i = 0; i < 3; ++i) {
        newHalfExtents[i] = glm::length(newAxes[i]) * oobb.halfSize[i];
        newAxes[i] = glm::normalize(newAxes[i]); 
    }
    glm::mat3 newOrientation = glm::mat3(newAxes[0], newAxes[1], newAxes[2]);
    return OOBB{ newCenter, newHalfExtents, newOrientation };
}
