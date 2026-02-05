#pragma once
#include <iomanip>
#include <iostream>
#include <chrono>

template <typename Clock>
class TimeImpl {
    double time;
public:
    TimeImpl() {
        *this = now();
    }

    template <typename T>
    requires std::is_convertible_v<T, double>
    explicit TimeImpl(T&& seconds) : time(static_cast<double>(seconds)) {}

    static TimeImpl seconds(const double secs) {
        return TimeImpl(secs);
    }

    static TimeImpl milliseconds(const size_t ms) {
        return TimeImpl(ms / 1'000);
    }
    
    static TimeImpl now() {
        static const auto start = Clock::now();

        auto now = Clock::now();

        return TimeImpl(std::chrono::duration<double>(now - start).count());
    }

    template <typename Value>
    TimeImpl operator / (const Value other) const {
        return TimeImpl(time / other);
    }

    TimeImpl operator - (const TimeImpl& other) const noexcept {
        return TimeImpl(time - other.time);
    }

    TimeImpl operator + (const TimeImpl& other) const noexcept {
        return TimeImpl(other.time + time);
    }

    TimeImpl& operator -= (const TimeImpl& other) noexcept {
        time -= other.time;
        return *this;
    }

    TimeImpl& operator += (const TimeImpl& other) noexcept {
        time += other.time;
        return *this;
    }

    bool operator > (const TimeImpl& other) const {
        return time > other.time;
    }

    bool operator < (const TimeImpl& other) const {
        return time < other.time;
    }

    bool operator >= (const TimeImpl& other) const {
        return time >= other.time;
    }

    bool operator <= (const TimeImpl& other) const {
        return time <= other.time;
    }

    bool operator == (const TimeImpl& other) const {
        return time == other.time;
    }

    bool operator != (const TimeImpl& other) const {
        return time != other.time;
    }

    double seconds() const noexcept {
        return time;
    }

    long long milliseconds() const noexcept {
        return static_cast<long long>(time * 1000.0);
    } 

    long long nanoseconds() const {
        return static_cast<long long>(time * 1000.0 * 1000.0 * 1000.0);
    }


    friend std::ostream& operator << (std::ostream& os, const TimeImpl& time) {
        os << std::fixed << std::setprecision(2);

        if (time.time < 0.001) {
            os << "0.00ms";
            return os;
        }
        if (time.time < 0.01) {
            os << "<0.01ms";
            return os;
        }
        os << time.time * 1000 << "ms";
        return os;
    }
};

using Time = TimeImpl<std::chrono::high_resolution_clock>;
using SteadyTime = TimeImpl<std::chrono::steady_clock>;