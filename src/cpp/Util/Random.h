#pragma once

#include <random>
#include <chrono>
#include <util/Concepts.h>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class Random {
public:
    static inline std::mt19937 gen = std::mt19937(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

    static double Double(const Numeric auto min, const Numeric auto max) {
        return std::uniform_real_distribution<double>(
            static_cast<double>(min), static_cast<double>(max)
            )(gen);
    }

    static float Float(const Numeric auto min, const Numeric auto max) {
        return std::uniform_real_distribution<float>(
            static_cast<float>(min), static_cast<float>(max)
            )(gen);
    }

    static int Int(const Numeric auto min, const Numeric auto max) {
        return std::uniform_int_distribution<int>(
            static_cast<int>(min), static_cast<int>(max)
        )(gen);
    }

    static unsigned Unsigned(const Numeric auto min, const Numeric auto max) {
        return std::uniform_int_distribution<unsigned>(
            static_cast<unsigned>(min), static_cast<unsigned>(max)
        )(gen);
    }

    static glm::vec2 Vec2(const glm::vec2& min, const glm::vec2& max) {
        return {
            Float(min.x, max.x),
            Float(min.y, max.y)
        };
    }

    static glm::vec3 Vec3(const glm::vec3& min, const glm::vec3& max) {
        return {
            Float(min.x, max.x),
            Float(min.y, max.y),
            Float(min.z, max.z)
        };
    }

    static glm::vec4 Vec4(const glm::vec4& min, const glm::vec4& max) {
        return {
            Float(min.x, max.x),
            Float(min.y, max.y),
            Float(min.z, max.z),
            Float(min.w, max.w)
        };
    }


    static glm::vec4 Color() {
        return {
            Float(0.f, 1.0f),
            Float(0.f, 1.0f),
            Float(0.f, 1.0f),
            Float(0.f, 1.0f)
        };
    }

    static glm::vec4 Color(const glm::vec4& color1, const glm::vec4& color2) {
        return {
            Float(color1.x, color2.x),
            Float(color1.y, color2.y),
            Float(color1.z, color2.z),
            Float(color1.w, color2.w)
        };
    }
};
