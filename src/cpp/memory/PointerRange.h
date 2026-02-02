#pragma once

template <typename T>
class PointerRange {
    T* first;
    T* last;
public:
    PointerRange(T* first, T* last) : first(first), last(last) {}

    T* begin() {
        return first;
    }

    T* end() {
        return last;
    }
};