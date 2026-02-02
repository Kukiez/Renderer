#pragma once
#include <cassert>
#include <stacktrace>
#include <string>
#include <iostream>

namespace cexpr {
    constexpr static auto is_debug = true;

    inline void printStackTrace() {
        const auto trace = std::stacktrace::current();
        std::cout << std::endl;
        for (auto it = trace.begin(); it != trace.end(); ++it) {
            std::string frame = it->description().substr(5);

            frame = frame.substr(0, frame.find_last_of('+'));

            std::string file = it->source_file();

            std::string line = std::to_string(it->source_line());
            std::cerr << file << "(" << line << ") \n  " << frame << std::endl << std::endl;
        }
    }

    template <typename F>
    void debug_only(F&& f) {
        if constexpr (is_debug) {
            f();
        }
    }

    template <typename F>
    void debug_assert(F&& f) {
        if constexpr (is_debug) {
            if (!f()) {
                printStackTrace();
                assert(false);
            }
        }
    }

    inline void debug_assert(bool boolean) {
        if constexpr (is_debug) {
            if (!boolean) {
                assert(false);
            }
        }
    }

    inline void require(bool condition) {
        if constexpr (is_debug) {
            if (!condition) {
                printStackTrace();
                assert(false);
            }
        }
    }

    template <typename ResultL>
     void require(bool condition, ResultL&& msg) {
        if constexpr (is_debug) {
            if (!condition) {
                printStackTrace();
                std::cout << std::endl;
                msg();
                assert(false);
            }
        }
    }
}