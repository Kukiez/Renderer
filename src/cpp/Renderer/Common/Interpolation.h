#pragma once

#include <string>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <util/glm_double.h>

using EasingFunction = double (*)(double);

enum class EasingType {
    LINEAR,
    SINE_IN,
    SINE_OUT,
    SINE_IN_OUT,
    CUBIC_IN,
    CUBIC_OUT,
    CUBIC_IN_OUT,
    CIRC_IN,
    CIRC_OUT,
    CIRC_IN_OUT,
    ELASTIC_IN,
    ELASTIC_OUT,
    ELASTIC_IN_OUT,
    BOUNCE_IN,
    BOUNCE_OUT,
    BOUNCE_IN_OUT,
    BACK_IN,
    BACK_OUT,
    BACK_IN_OUT,
    QUADRATIC_IN,
    QUADRATIC_OUT,
    QUADRATIC_IN_OUT,
    QUARTIC_IN,
    QUARTIC_OUT,
    QUARTIC_IN_OUT,
    QUINT_IN,
    QUINT_OUT,
    QUINT_IN_OUT,
    EXPO_IN,
    EXPO_OUT,
    EXPO_IN_OUT
};

enum class InterpolationType {
    LERP, HERMITE
};

struct Easing {
    using easing_type = double;

    using EasingFunction = easing_type (*)(easing_type);
    EasingFunction function;

    static easing_type sine_in(const easing_type x) {
        return 1 - glm::cos((x * glm::pi<easing_type>()) / 2);
    }

    static easing_type sine_out(const easing_type x) {
        return sin((x * glm::pi<easing_type>()) / 2);
    }

    static easing_type sine_in_out(const easing_type x) {
        return -(cos(glm::pi<easing_type>() * x) - 1) / 2;
    }

    static easing_type linear(const easing_type x) {
        return x;
    }

    static easing_type cubic_in(const easing_type x) {
        return x * x * x;
    }

    static easing_type cubic_out(const easing_type x) {
        const easing_type f = (x - 1);
        return f * f * f + 1;
    }

    static easing_type cubic_in_out(const easing_type x) {
        const easing_type pow = -2 * x + 2;
        return x < 0.5 ? 4 * x * x * x : 1 - (pow * pow * pow) / 2;
    }

    static easing_type circ_in(const easing_type x) {
        return 1 - sqrt(1 - x * x);
    }

    static easing_type circ_out(const easing_type x) {
        return sqrt((2 - x) * x);
    }

    static easing_type circ_in_out(const easing_type x) {
        return x < 0.5 ? (1 - sqrt(1 - 4 * x * x)) / 2 : (sqrt(-(2 * x - 3) * (2 * x - 1)) + 1) / 2;
    }

    static easing_type elastic_in(const easing_type x) {
        return sin(13 * glm::pi<easing_type>() * x) * pow(2, 10 * (x - 1));
    }

    static easing_type elastic_out(const easing_type x) {
        return sin(-13 * glm::pi<easing_type>() * (x + 1)) * pow(2, -10 * x) + 1;
    }

    static easing_type elastic_in_out(const easing_type x) {
        return x < 0.5 ? sin(13 * glm::pi<easing_type>() * 2 * x) * pow(2, 10 * (2 * x - 1)) / 2 : (sin(-13 * glm::pi<easing_type>() * (2 * x - 1 + 1)) * pow(2, -10 * (2 * x - 1)) + 2) / 2;
    }

    static easing_type bounce_out(easing_type x) {
        constexpr easing_type n1 = 7.5625f;
        constexpr easing_type d1 = 2.75f;

        if (x < 1 / d1) {
            return n1 * x * x;
        }
        if (x < 2 / d1) {
            return n1 * (x -= 1.5 / d1) * x + 0.75;
        }
        if (x < 2.5 / d1) {
            return n1 * (x -= 2.25 / d1) * x + 0.9375;
        }
        return n1 * (x -= 2.625 / d1) * x + 0.984375;
    }

    static easing_type bounce_in(const easing_type x) {
        return 1 - bounce_out(1 - x);
    }

    static easing_type bounce_in_out(const easing_type x) {
        return x < 0.5 ? (1 - bounce_out(1 - 2 * x)) / 2 : (1 + bounce_out(2 * x - 1)) / 2;
    }

    static easing_type back_in(const easing_type x) {
        return x * x * x - x * sin(x * glm::pi<easing_type>());
    }

    static easing_type back_out(const easing_type x) {
        const easing_type f = 1 - x;
        return 1 + f * f * f - f * sin(f * glm::pi<easing_type>());
    }

    static easing_type back_in_out(const easing_type x) {
        const easing_type f = 2 * x;
        return x < 0.5 ? (f * f * f - f * sin(f * glm::pi<easing_type>())) / 2 : (1 + (f - 2) * (f - 2) * (f - 2) - f * sin(f * glm::pi<easing_type>())) / 2;
    }

    static easing_type quadratic_in(const easing_type x) {
        return x * x;
    }

    static easing_type quadratic_out(const easing_type x) {
        return -x * (x - 2);
    }

    static easing_type quadratic_in_out(const easing_type x) {
        return x < 0.5 ? 2 * x * x : -1 + (4 - 2 * x) * x;
    }

    static easing_type quartic_in(const easing_type x) {
        return x * x * x * x;
    }

    static easing_type quartic_out(const easing_type x) {
        const easing_type f = (x - 1);
        return f * f * f * (1 - x) + 1;
    }

    static easing_type quint_in(const easing_type x) {
        return x * x * x * x * x;
    }

    static easing_type quint_out(const easing_type x) {
        const easing_type f = (x - 1);
        return f * f * f * f * (1 + x) + 1;
    }

    static easing_type quint_in_out(const easing_type x) {
        return x < 0.5 ? 16 * x * x * x * x * x : 1 - pow(-2 * x + 2, 5) / 2;
    }

    static easing_type quartic_in_out(const easing_type x) {
        return x < 0.5 ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2;
    }

    static easing_type expo_in(const easing_type x) {
        return x == 0 ? 0 : pow(2, 10 * (x - 1));
    }

    static easing_type expo_out(const easing_type x) {
        return x == 1 ? 1 : 1 - pow(2, -10 * x);
    }

    static easing_type expo_in_out(const easing_type x) {
        return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ? pow(2, 20 * x - 10) / 2 : (2 - pow(2, -20 * x + 10)) / 2;
    }
};

inline double (*getEasingFunction(const EasingType type))(double) {
    switch (type) {
        case EasingType::SINE_IN: return Easing::sine_in;
        case EasingType::SINE_OUT: return Easing::sine_out;
        case EasingType::SINE_IN_OUT: return Easing::sine_in_out;
        case EasingType::CUBIC_IN: return Easing::cubic_in;
        case EasingType::CUBIC_OUT: return Easing::cubic_out;
        case EasingType::CUBIC_IN_OUT: return Easing::cubic_in_out;
        case EasingType::CIRC_IN: return Easing::circ_in;
        case EasingType::CIRC_OUT: return Easing::circ_out;
        case EasingType::CIRC_IN_OUT: return Easing::circ_in_out;
        case EasingType::ELASTIC_IN: return Easing::elastic_in;
        case EasingType::ELASTIC_OUT: return Easing::elastic_out;
        case EasingType::ELASTIC_IN_OUT: return Easing::elastic_in_out;
        case EasingType::BOUNCE_IN: return Easing::bounce_in;
        case EasingType::BOUNCE_OUT: return Easing::bounce_out;
        case EasingType::BOUNCE_IN_OUT: return Easing::bounce_in_out;
        case EasingType::BACK_IN: return Easing::back_in;
        case EasingType::BACK_OUT: return Easing::back_out;
        case EasingType::BACK_IN_OUT: return Easing::back_in_out;
        case EasingType::QUADRATIC_IN: return Easing::quadratic_in;
        case EasingType::QUADRATIC_OUT: return Easing::quadratic_out;
        case EasingType::QUADRATIC_IN_OUT: return Easing::quadratic_in_out;
        case EasingType::QUARTIC_IN: return Easing::quartic_in;
        case EasingType::QUARTIC_OUT: return Easing::quartic_out;
        case EasingType::QUARTIC_IN_OUT: return Easing::quartic_in_out;
        case EasingType::QUINT_IN: return Easing::quint_in;
        case EasingType::QUINT_OUT: return Easing::quint_out;
        case EasingType::QUINT_IN_OUT: return Easing::quint_in_out;
        case EasingType::EXPO_IN: return Easing::expo_in;
        case EasingType::EXPO_OUT: return Easing::expo_out;
        case EasingType::EXPO_IN_OUT: return Easing::expo_in_out;
        default: return Easing::linear;
    }
}

inline EasingType stringToEasingType(const std::string& str) {
    static const std::unordered_map<std::string, EasingType> easingMap = {
        {"LINEAR", EasingType::LINEAR},
        {"SINE_IN", EasingType::SINE_IN},
        {"SINE_OUT", EasingType::SINE_OUT},
        {"SINE_IN_OUT", EasingType::SINE_IN_OUT},
        {"CUBIC_IN", EasingType::CUBIC_IN},
        {"CUBIC_OUT", EasingType::CUBIC_OUT},
        {"CUBIC_IN_OUT", EasingType::CUBIC_IN_OUT},
        {"CIRC_IN", EasingType::CIRC_IN},
        {"CIRC_OUT", EasingType::CIRC_OUT},
        {"CIRC_IN_OUT", EasingType::CIRC_IN_OUT},
        {"ELASTIC_IN", EasingType::ELASTIC_IN},
        {"ELASTIC_OUT", EasingType::ELASTIC_OUT},
        {"ELASTIC_IN_OUT", EasingType::ELASTIC_IN_OUT},
        {"BOUNCE_IN", EasingType::BOUNCE_IN},
        {"BOUNCE_OUT", EasingType::BOUNCE_OUT},
        {"BOUNCE_IN_OUT", EasingType::BOUNCE_IN_OUT},
        {"BACK_IN", EasingType::BACK_IN},
        {"BACK_OUT", EasingType::BACK_OUT},
        {"BACK_IN_OUT", EasingType::BACK_IN_OUT},
        {"QUADRATIC_IN", EasingType::QUADRATIC_IN},
        {"QUADRATIC_OUT", EasingType::QUADRATIC_OUT},
        {"QUADRATIC_IN_OUT", EasingType::QUADRATIC_IN_OUT},
        {"QUARTIC_IN", EasingType::QUARTIC_IN},
        {"QUARTIC_OUT", EasingType::QUARTIC_OUT},
        {"QUARTIC_IN_OUT", EasingType::QUARTIC_IN_OUT},
        {"QUINT_IN", EasingType::QUINT_IN},
        {"QUINT_OUT", EasingType::QUINT_OUT},
        {"QUINT_IN_OUT", EasingType::QUINT_IN_OUT},
        {"EXPO_IN", EasingType::EXPO_IN},
        {"EXPO_OUT", EasingType::EXPO_OUT},
        {"EXPO_IN_OUT", EasingType::EXPO_IN_OUT}
    };

    if (const auto it = easingMap.find(str); it != easingMap.end()) {
        return it->second;
    }
    return easingMap.at("LINEAR");
}

struct Interpolate {
    template <typename T>
    static auto step(T&& start, T&& end, const double t, const EasingFunction easing = Easing::linear) -> std::decay_t<T> {
        const double easedT = easing(t);
        return easedT >= 0.5f ? std::forward<T>(end) : std::forward<T>(start);
    }


    template <typename T1, typename T2>
    static auto lerp(T1&& start, T2&& end, const double t, const EasingFunction easing = Easing::linear) -> std::decay_t<T1> {
        double easedT = easing(t);
        return std::forward<T1>(start) + (std::forward<T2>(end) - std::forward<T1>(start)) * easedT;
    }

    template <typename T>
    static auto slerp(T&& start, T&& end, const double t, const double theta, const EasingFunction easing = Easing::linear) -> std::decay_t<T> {
        const double easedT = easing(t);
        return std::forward<T>(start) * (std::sin((1 - easedT) * theta) / std::sin(theta)) +
               std::forward<T>(end) * (std::sin(easedT * theta) / std::sin(theta));
    }

    static glm::quat slerp(const glm::quat& start, const glm::quat& end, double t, EasingFunction easing = Easing::linear) {
        const double easedT = easing(t);

        return glm::slerp(start, end, static_cast<float>(easedT));

        // double dotProduct = glm::dot(start, end);
        //
        // glm::quat endAdjusted = end;
        //
        // if (dotProduct < 0.0f) {
        //     endAdjusted = -end;
        //     dotProduct = -dotProduct;
        // }
        //
        // if (dotProduct > 0.9995f) {
        //     return glm::normalize(glm::mix(start, endAdjusted, static_cast<float>(easedT)));
        // }
        //
        // const double theta = std::acos(dotProduct);
        // const double sinTheta = std::sin(theta);
        //
        // const double a = std::sin((1.0 - easedT) * theta) / sinTheta;
        // const double b = std::sin(easedT * theta) / sinTheta;
        //
        // return glm::normalize(
        //     static_cast<float>(a) * start +
        //     static_cast<float>(b) * endAdjusted
        // );
    }

    template <typename T>
    static auto smooth_step(T&& start, T&& end, const double t, const EasingFunction easing = Easing::linear) -> std::decay_t<T> {
        const double easedT = easing(t);
        double smoothFactor = easedT * easedT * (3 - 2 * easedT);
        return lerp(std::forward<T>(start), std::forward<T>(end), smoothFactor, Easing::linear);
    }


    template <typename T>
    static auto bezier(T&& p0, T&& p1, T&& p2, T&& p3, const double t, const EasingFunction easing = Easing::linear) -> std::decay_t<T> {
        const double easedT = easing(t);
        return std::pow(1 - easedT, 3) * std::forward<T>(p0) +
               3 * std::pow(1 - easedT, 2) * easedT * std::forward<T>(p1) +
               3 * (1 - easedT) * std::pow(easedT, 2) * std::forward<T>(p2) +
               std::pow(easedT, 3) * std::forward<T>(p3);
    }

    template <typename T>
    static auto catmull_rom(T&& p0, T&& p1, T&& p2, T&& p3, const double t, const EasingFunction easing = Easing::linear) -> std::decay_t<T> {
        double easedT = easing(t);
        return 0.5f * (
            (2 * std::forward<T>(p1)) +
            (-std::forward<T>(p0) + std::forward<T>(p2)) * easedT +
            (2 * std::forward<T>(p0) - 5 * std::forward<T>(p1) + 4 * std::forward<T>(p2) - std::forward<T>(p3)) * easedT * easedT +
            (-std::forward<T>(p0) + 3 * std::forward<T>(p1) - 3 * std::forward<T>(p2) + std::forward<T>(p3)) * easedT * easedT * easedT
        );
    }

    template <typename T0, typename T1, typename T2, typename T3, typename P>
    static auto hermite(T0&& p0, T1&& p1, T2&& m0, T3&& m1, const P t, const EasingFunction easing = Easing::linear) -> std::decay_t<T0> {
        const P easedT = static_cast<P>(easing(t));
        const P t2 = static_cast<P>(easedT * easedT);
        const P t3 = static_cast<P>(t2 * easedT);

        return (t3 * 2 - t2 * 3 + 1) * std::forward<T0>(p0) +
               (t3 - 2 * t2 + easedT) * std::forward<T2>(m0) +
               (t3 * -2 + t2 * 3) * std::forward<T1>(p1) +
               (t3 - t2) * std::forward<T3>(m1);
    }

};

struct FadeEffect {
    double duration;
    EasingFunction easing;

    explicit FadeEffect(const double duration = 0, const EasingFunction function = Easing::linear) : duration(duration), easing(function) {}
};

using FadeIn = FadeEffect;
using FadeOut = FadeEffect;