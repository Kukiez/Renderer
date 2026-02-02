#pragma once
#include <atomic>
#include "type_info.h"

namespace mem {
    template <typename T>
    concept CastableToUll = requires(T t) {
        static_cast<size_t>(t);
    };

    template <typename Impl>
    concept IsAtomicByteArena = requires(Impl impl, char* charPtr, size_t capacity)
    {
        impl.onAllocate(capacity);
        impl.onDeallocate(charPtr, capacity);
    };

    template <IsAtomicByteArena Impl>
    class atomic_byte_arena_traits : public Impl {
        char* myData = nullptr;
        std::atomic<size_t> myNext = 0;
        size_t myCapacity = 0;
    public:
        atomic_byte_arena_traits() = default;

        template <CastableToUll Capacity>
        explicit atomic_byte_arena_traits(const Capacity capacity) : myCapacity(static_cast<size_t>(capacity)) {
            myData = Impl::onAllocate(capacity);
        }

        atomic_byte_arena_traits(const atomic_byte_arena_traits&) = delete;
        atomic_byte_arena_traits& operator=(const atomic_byte_arena_traits&) = delete;

        atomic_byte_arena_traits(atomic_byte_arena_traits&& other) noexcept : myData(other.myData), myNext(other.myNext.load()), myCapacity(other.myCapacity) {
            other.myData = nullptr;
            other.myNext = 0;
            other.myCapacity = 0;
        }

        atomic_byte_arena_traits& operator=(atomic_byte_arena_traits&& other) noexcept {
            if (this != &other) {
                myData = other.myData;
                myNext = other.myNext.load();
                myCapacity = other.myCapacity;

                other.myData = nullptr;
                other.myNext = 0;
                other.myCapacity = 0;
            }
            return *this;
        }

        ~atomic_byte_arena_traits() {
            Impl::onDeallocate(myData, myCapacity);
        }

        void* allocate(const size_t bytes, const size_t align) {
            const size_t size = bytes;
            const size_t alignment = align;

            while (true) {
                size_t old_offset = myNext.load(std::memory_order_acquire);
                const size_t aligned_offset = (old_offset + alignment - 1) & ~(alignment - 1);
                const size_t new_offset = aligned_offset + size;

                if (new_offset > myCapacity) {
                    return nullptr;
                }

                if (myNext.compare_exchange_weak(
                    old_offset, new_offset,
                    std::memory_order_acq_rel, std::memory_order_relaxed)
                ) {
                    return myData + aligned_offset;
                }
            }
        }

        void* allocate(const type_info* type, const size_t count, const size_t aligned = -1) {
            return allocate(type->size * count, aligned != -1 ? aligned : type->align);
        }

        void reset() {
            myNext.store(0, std::memory_order_release);
        }

        void* data() {
            return myData;
        }

        template <typename T>
        T* as() {
            return static_cast<T*>(data());
        }

        const void* data() const {
            return myData;
        }

        size_t next() const {
            return myNext.load(std::memory_order_relaxed);
        }

        size_t remaining() const {
            return myCapacity - myNext.load(std::memory_order_relaxed);
        }

        size_t capacity() const {
            return myCapacity;
        }
    };

    struct atomic_byte_arena_traits_default {
        static char* onAllocate(size_t capacity) {
            return static_cast<char *>(operator new(capacity, std::align_val_t{64}));
        }

        static void onDeallocate(char* ptr, size_t) {
            operator delete(ptr, std::align_val_t{64});
        }
    };

    using atomic_byte_arena = atomic_byte_arena_traits<atomic_byte_arena_traits_default>;

    struct chained_atomic_byte_arena {
        tbb::concurrent_vector<atomic_byte_arena> arenas;
    };
}
