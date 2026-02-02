#pragma once

// atomic for popping, non-atomic for pushing
template <typename T>
class AtomicPopList {
    std::vector<T> list{};
    std::atomic<long long> tail{};
public:
    AtomicPopList() = default;

    bool tryPop(T& outT) {
        const auto idx = tail.fetch_sub(1);

        if (idx <= 0) {
            return false;
        }
        outT = list[idx];
        return true;
    }

    template <typename... Args>
    void push(Args&&... args) {
        list.emplace_back(std::forward<Args>(args)...);
        tail = list.size() - 1;
    }

    void clear() {
        tail = 0;
        list.clear();
    }
};