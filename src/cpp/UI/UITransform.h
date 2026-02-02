#pragma once

namespace ui {
    struct UITransform {
        glm::vec2 position{};
        glm::vec2 size{0.f};

        UITransform() = default;

        constexpr UITransform(const glm::vec2& position, const glm::vec2& scale = glm::vec2(0)) : position(position), size(scale) {}

        UITransform& operator -= (const UITransform& other) {
            position -= other.position;
            size -= other.size;
            return *this;
        }

        UITransform& operator += (const UITransform& other) {
            position += other.position;
            size += other.size;
            return *this;
        }

        UITransform operator / (const float scalar) const {
            return UITransform{ position / scalar, size / scalar };
        }

        UITransform  operator*(double result_weight) const {
            return UITransform{ position * result_weight, size * result_weight };
        }
    };

    struct UIBounds {
        glm::vec2 min, max;

        constexpr UIBounds() = default;

        constexpr UIBounds(const glm::vec2& min, const glm::vec2& max) : min(min), max(max) {}
        constexpr UIBounds(const float minX, const float minY, const float maxX, const float maxY) : min(minX, minY), max(maxX, maxY) {}

        bool intersects(const UIBounds& other) const {
            return !(other.min.x > max.x || other.max.x < min.x ||
                     other.min.y > max.y || other.max.y < min.y);
        }

        bool contains(const UIBounds& other) const {
            return !(other.min.x < min.x || other.max.x > max.x ||
                     other.min.y < min.y || other.max.y > max.y);
        }

        bool contains(const glm::vec2& point) const {
            return !(point.x < min.x || point.x > max.x ||
                     point.y < min.y || point.y > max.y);
        }

        glm::vec2 size() const {
            return max - min;
        }

        glm::vec2 center() const {
            return (min + max) / 2.f;
        }

        friend std::ostream& operator<<(std::ostream& os, const UIBounds& bounds) {
            return os << "{min: (" << bounds.min << "), max: (" << bounds.max << ")}";
        }
    };
}