#pragma once
#include <algorithm>
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <Renderer/Common/Interpolation.h>


namespace curves {
    template <typename KFT, typename Precision>
    struct Keyframe {
        KFT value;
        Precision time;

        KFT inTangent;
        KFT outTangent;

        explicit Keyframe(const std::pair<KFT, Precision> &value)
        : value(value.first), time(value.second), inTangent(), outTangent() {}

        Keyframe(const KFT val, const KFT in, const KFT out, Precision time)
            : value(val), time(time), inTangent(in), outTangent(out) {}
    };

    template <typename KFType, typename Precision>
    struct curve_traits {
        using type = KFType;
        using precision = Precision;
    };

    template <typename traits>
    class Curve {
        using KeyframeT = Keyframe<typename traits::type, typename traits::precision>;

        std::vector<KeyframeT> keyframes;
        EasingFunction easing = Easing::linear;
        InterpolationType interpolation = InterpolationType::LERP;
    public:
        using keyframe_type = typename traits::type;
        using precision_type = typename traits::precision;

        Curve() = default;

        explicit Curve(const std::initializer_list<std::pair<keyframe_type, precision_type>> kfs) {
            for (const auto& k : kfs) {
                add(k.first, k.second);
            }
        }

        void setEasingFunction(EasingFunction easing) {
            this->easing = easing;
        }

        void setInterpolationType(InterpolationType interpolation) {
            this->interpolation = interpolation;
        }

        void generateTangentsHermite() {
            for (size_t i = 1; i < keyframes.size() - 1; ++i) {
                const auto& prev = keyframes[i - 1];
                const auto& next = keyframes[i + 1];
                const auto& curr = keyframes[i];

                precision_type dt = next.time - prev.time;
                keyframe_type delta = (next.value - prev.value) / dt;

                keyframes[i].inTangent  = delta;
                keyframes[i].outTangent = delta;
            }
        }

        typename std::vector<KeyframeT>::iterator nextAt(const precision_type at) {
            return std::lower_bound(keyframes.begin(), keyframes.end(), at,
                [](const KeyframeT& kf, const precision_type t) { return kf.time < t; });
        }

        void add(const keyframe_type& value, const precision_type at) {
            auto pos = nextAt(at);
            keyframes.emplace(pos, std::make_pair(value, at));
        }

        void add(const keyframe_type& value, const keyframe_type& inTangent, const keyframe_type& outTangent, const precision_type at) {
            auto pos = nextAt(at);
            keyframes.emplace(pos, Keyframe(value, inTangent, outTangent, at));
        }

        keyframe_type at(const precision_type time) const {
            static size_t NO_HINT = 0;

            if (keyframes.empty()) return keyframe_type{};

            if (time <= keyframes.front().time) return keyframes.front().value;
            if (time >= keyframes.back().time) return keyframes.back().value;

            keyframe_type result;
            interpolate(0, keyframes.size() - 1, time, NO_HINT, result);
            return result;
        }

        keyframe_type at(const precision_type time, size_t& hint) const {
            if (keyframes.empty()) [[unlikely]] return keyframe_type{};
            if (time <= keyframes.front().time) return keyframes.front().value;
            if (time >= keyframes.back().time) return keyframes.back().value;

            keyframe_type result;
            if (interpolate(hint, keyframes.size() - 1, time, hint, result)) [[likely]] return result;
            interpolate(0, hint, time, hint, result);
            return result;
        }

        void eraseFrom(const precision_type at) {
            auto pos = nextAt(at);
            keyframes.erase(pos, keyframes.end());
        }

        bool empty() const {
            return keyframes.empty();
        }

        void clear() {
            keyframes.clear();
        }

        KeyframeT& first() {
            return keyframes.front();
        }

        KeyframeT& last() {
            return keyframes.back();
        }

        const KeyframeT& last() const {
            return keyframes.back();
        }

        precision_type duration() const {
            return last().time;
        }

        size_t size() const {
            return keyframes.size();
        }
    private:
        keyframe_type lerp(const KeyframeT& a, const KeyframeT& b, const precision_type time) const {
            const precision_type t = easing(
                (time - a.time) / (b.time - a.time)
            );
            if constexpr (std::is_same_v<keyframe_type, glm::quat>) {
                return slerp(a.value, b.value, static_cast<float>(t));
            } else {
                return Interpolate::lerp(a.value, b.value, t);
            }
        }

        keyframe_type hermite(const KeyframeT& a, const KeyframeT& b, const precision_type time) const {
            const precision_type duration = b.time - a.time;

            return Interpolate::hermite(
                a.value, b.value,
                a.outTangent * duration,
                b.inTangent * duration,
                static_cast<float>((time - a.time) / duration),
                easing
            );
        }

        bool interpolate(const size_t start, const size_t end,
            const precision_type time, size_t& hint,
            keyframe_type& outResult) const
        {
            for (size_t i = start; i < end; ++i) {
                const auto& a = keyframes[i];
                const auto& b = keyframes[i + 1];

                if (time >= a.time && time <= b.time) {
                    hint = i;

                    switch (interpolation) {
                        case InterpolationType::LERP:
                            outResult = lerp(a, b, time);
                        break;
                        case InterpolationType::HERMITE:
                            outResult = hermite(a, b, time);
                        break;
                    }
                    return true;
                }
            }
            return false;
        }
    };

    template <typename K, typename V, typename Eq>
    class EquationCurve {
        Eq equation;
    public:
        constexpr explicit EquationCurve(const Eq eq) : equation(eq) {}

        void setEquation(const Eq eq) {
            equation = eq;
        }

        V at(const K& k) const {
            return equation(k);
        }

        V operator()(const K& k) const {
            return at(k);
        }
    };

}

using FCurve4 = curves::Curve<curves::curve_traits<glm::vec4, double>>;
using FCurve3 = curves::Curve<curves::curve_traits<glm::vec3, double>>;
using QCurve = curves::Curve<curves::curve_traits<glm::quat, double>>;

template <typename K, typename V, typename Eq = V(*)(const K&)>
using ECurve = curves::EquationCurve<K, V, Eq>;