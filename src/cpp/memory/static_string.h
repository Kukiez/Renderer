#pragma once

namespace mem {
    struct static_string {
        const char* value;

        template <int N>
        constexpr static_string(const char (&str)[N]) {
            value = str;
        }

        constexpr const char* c_str() const { return value; }

        constexpr const char* data() const { return value; }
    };
}
