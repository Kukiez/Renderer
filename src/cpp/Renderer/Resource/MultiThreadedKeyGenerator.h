#pragma once

template <typename Key, typename KeyIndexer = decltype([](Key k) { return k.index(); })>
struct MultiThreadedKeyGenerator {
    using KeyIntegerType = cexpr::function_return_t<KeyIndexer>;

    std::atomic<KeyIntegerType> next = 1;

    size_t tlsPortion = 5;

    struct ThreadLocalKeyGenerator {
        MultiThreadedKeyGenerator* generator{};
        KeyIntegerType current = 0;
        KeyIntegerType max = 0;

        std::vector<KeyIntegerType> freeKeys{};

        KeyIntegerType generate() {
            if (freeKeys.empty()) {
                if (current == max) {
                    auto [newCurr, newMax] = generator->getNewKeyRange();
                    current = newCurr;
                    max = newMax;
                }
                return current++;
            }
            return freeKeys.back();
        }

        void free(const KeyIntegerType key) {
            freeKeys.push_back(key);
        }
    };

    tbb::enumerable_thread_specific<ThreadLocalKeyGenerator> generators;

    MultiThreadedKeyGenerator() : generators{} {
        generators = tbb::enumerable_thread_specific<ThreadLocalKeyGenerator>(this, 0, 0, std::vector<KeyIntegerType>());
    }

    std::pair<KeyIntegerType, KeyIntegerType> getNewKeyRange() {
        KeyIntegerType current = next.fetch_add(tlsPortion);

        return {current, current + tlsPortion};
    }

    template <typename... Args>
    Key generate(Args&&... args) {
        KeyIntegerType key = generators.local().generate();

        return Key{key, std::forward<Args>(args)...};
    }

    void release(const Key key) {
        generators.local().free(KeyIndexer{}(key));
    }
};