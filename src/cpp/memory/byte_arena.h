#pragma once
#include "type_info.h"
#include <type_traits>
#include "TypeOps.h"
#include "alloc.h"
#include "memory/memory.h"

namespace mem {
    class bytes_required {
        size_t bytes = 0;
    public:
        bytes_required() = default;

        template <std::integral T>
        constexpr bytes_required(T startingBytes) : bytes(static_cast<size_t>(startingBytes)) {}


        operator size_t() const {
            return bytes;
        }

        constexpr void include(const type_info* type, size_t count) {
            size_t offsetRequired = mem::padding(type, bytes);
            bytes += offsetRequired;
            bytes += type->size * count;
        }

        constexpr void include(size_t forBytes, size_t align) {
            size_t offsetRequired = mem::padding(bytes, align);
            bytes += offsetRequired;
            bytes += forBytes;
        }

        constexpr void add_padding(size_t paddingBytes) {
            bytes += paddingBytes;
        }

        constexpr size_t get() const {
            return bytes;
        }

        friend std::ostream& operator << (std::ostream& os, const bytes_required b) {
            os << b.bytes;
            return os;
        }
    };

    template <typename T, typename Arena>
    struct byte_arena_adaptor;

    template <
        typename ReallocSchema = same_alloc_schema,
        size_t BaseAlign = alignof(std::max_align_t)
    >
    class byte_arena {
        void deallocate() const {
            operator delete(memory, std::align_val_t{BaseAlign});
        }

        static char* allocate(const size_t capacity) {
            return static_cast<char*>(operator new(capacity, std::align_val_t{BaseAlign}));
        }

        char* memory = nullptr;
        size_t next = 0;
        size_t capacity_ = 0;
        byte_arena* nextArena = nullptr;
        ReallocSchema reallocSchema;
    public:
        template <typename T>
        using Adaptor = byte_arena_adaptor<T, byte_arena>;

        using IsByteArena = std::true_type;

        byte_arena() = default;

        template <typename T>
        requires requires (T t) { static_cast<size_t>(t); }
        explicit byte_arena(T capacity) : memory(allocate(static_cast<size_t>(capacity))), capacity_(static_cast<size_t>(capacity)) {}
        
        byte_arena(const byte_arena&) = delete;
        byte_arena& operator=(const byte_arena&) = delete;

        byte_arena(byte_arena&& other) noexcept 
        : nextArena(other.nextArena), memory(other.memory), next(other.next), capacity_(other.capacity_) {
            other.memory = nullptr;
            other.next = 0;
            other.nextArena = nullptr;
            other.capacity_ = 0;
        }

        byte_arena& operator=(byte_arena&& other) noexcept {
            if (this != &other) {
                deallocate();
                delete nextArena;
                memory = other.memory;
                next = other.next;
                nextArena = other.nextArena;
                capacity_ = other.capacity_;
                other.memory = nullptr;
                other.next = 0;
                other.nextArena = nullptr;
                other.capacity_ = 0;
            }
            return *this;
        }

        ~byte_arena() {
            deallocate();
            delete nextArena;
        }

        void* allocate(const size_t bytes, const size_t alignment) {
            type_info type;
            type.align = alignment;
            type.size = bytes;
            return allocate(&type, 1);
        }
        
        void* allocate(const type_info* type, const size_t count) {
            byte_arena* self = this;

            if (!self->memory) [[unlikely]] {
                self->initialize(type, count);
                self->next += type->size * count;
                return self->memory;
            }

            while (self) {
                const size_t offsetRequired = padding(type, self->next);
                const size_t typeBytes = type->size * count;

                char* mem = self->memory + self->next + offsetRequired;

                if (mem + typeBytes > self->memory + self->capacity_) [[unlikely]] {
                    if (!self->nextArena) {
                        self->nextArena = new byte_arena(self->reallocSchema.grow(self->capacity_, count * type->size));
                    }
                    self = self->nextArena;
                    continue;
                }
                self->next += offsetRequired + type->size * count;
                return mem;
            }
            std::unreachable();
        }

        template <typename T>
        T* allocate(size_t count) {
            return static_cast<T*>(allocate(type_info::of<T>(), count));
        }

        void initialize(const size_t bytes) {
            deallocate();
            memory = allocate(bytes);
            capacity_ = bytes;
        }

        void initialize(const bytes_required bytes) {
            deallocate();
            memory = allocate(bytes.get());
            capacity_ = bytes.get();
        }

        void initialize(const type_info* type, size_t count) {
            deallocate();
            memory = allocate(type->size * count);
            capacity_ = type->size * count;
        }
        
        bool is_initialized() const {
            return memory != nullptr;
        }

        void* allocate_or_fail(const type_info* type, size_t count) {
            const size_t offsetRequired = padding(type, next);
            const size_t typeBytes = type->size * count;

            if (!memory) {
                initialize(type, count);
                next += offsetRequired + type->size * count;
                return memory;
            }
            char* mem = memory + next + offsetRequired;

            if (mem + typeBytes > memory + capacity_) [[unlikely]] {
                return nullptr;
            }
            next += offsetRequired + type->size * count;
            return mem;
        }

        template <typename T>
        T* allocate_or_fail(const size_t count) {
            return static_cast<T*>(allocate_or_fail(type_info::of<T>(), count));
        }

        void align_next_to(size_t alignment) {
            size_t offsetRequired = mem::padding(next, alignment);
            next += offsetRequired;
            std::cout << "Aligned Next: " << next << std::endl;
        }

        /**
         * This function invalidates any memory handed by this instance
         */
        void reserve_compact(const bytes_required bytes) {
            destroy_adjacent();
            if (capacity_ < bytes.get()) {
                const size_t capacity = total_capacity();
                initialize(std::max(capacity, bytes.get()));
            }
        }

        bool reserve(const bytes_required bytes) {
            if (!memory) {
                initialize(bytes);
                return true;
            }

            if (next + bytes.get() > capacity_) {
                if (nextArena) {
                    return nextArena->reserve(bytes);
                }
                nextArena = new byte_arena(reallocSchema.grow(capacity_, bytes.get()));
                return false;
            }
            return true;
        }

        /**
         * This function invalidates any memory handed by this instance
         */
        void reset_compact() {
            size_t totalCapacity = 0;

            if (!nextArena) {
                next = 0;
                return;
            }

            auto current = this;

            while (current->nextArena) {
                totalCapacity += current->nextArena->capacity_;
                current = current->nextArena;
            }

            if (totalCapacity != 0) {
                totalCapacity += capacity_;
                deallocate();
                destroy_adjacent();
                memory = allocate(totalCapacity);
            }
            next = 0;
        }

        void* copy_all_memory_to(void* dst, const mem::type_info* type) {
            auto arena = this;

            char* dstChar = static_cast<char*>(dst);

            while (arena) {
                mem::copy(type, dstChar, arena->memory, arena->next / type->size);
                dstChar += arena->capacity_;
                arena = arena->nextArena;
            }
            return dstChar;
        }

        /**
         * This function invalidates any memory handed by this instance
         */
        void reset_shrink() {
            destroy_adjacent();
            next = 0;
        }

        void reset() {
            next = 0;
            auto current = this;

            if (current->nextArena)
                current->nextArena->reset();
        }

        void destroy_adjacent() {
            delete nextArena;
            nextArena = nullptr;
        }

        bool has_for(const mem::type_info* type, const size_t count) const {
            const bool has = next + type->size * count <= capacity_;

            if (has) return true;

            if (nextArena) {
                return nextArena->has_for(type, count);
            }
            return false;
        }

        size_t bytes_used() const {
            size_t used = next;
            auto self = nextArena;
            while (self) {
                used += self->next;
                self = self->nextArena;
            }
            return used;
        }

        size_t total_capacity() {
            size_t cap = capacity_;

            auto self = nextArena;
            
            while (self) {
                cap += self->capacity_;
                self = self->nextArena;
            }
            return cap;
        }

        char* data() const {
            return memory;
        }

        void destroy() {
            *this = byte_arena{};
        }

        bool does_pointer_belong_here(const void* ptr) const {
            auto self = this;

            while (self) {
                auto * begin = reinterpret_cast<const uint8_t*>(self->memory);
                const uint8_t* end = begin + self->capacity_;

                if (ptr >= begin && ptr < end) {
                    return true;
                }

                self = self->nextArena;
            }

            return false;
        }
    };

    class allocation_wrapper {
        void* mem = nullptr;
        const mem::type_info* type;
    };

    template <
        typename ReallocSchema = same_alloc_schema,
        size_t BaseAlign = alignof(std::max_align_t)
    >
    byte_arena<ReallocSchema, BaseAlign> create_byte_arena(const size_t capacity) {
        return mem::byte_arena<ReallocSchema, BaseAlign>{
            capacity
        };
    }

    template <typename T, typename Arena = byte_arena<>>
    struct byte_arena_adaptor {
        using value_type = T;
        Arena* arena = nullptr;

        byte_arena_adaptor() = default;

        byte_arena_adaptor(Arena* arena) : arena(arena) {}
        
        byte_arena_adaptor(const byte_arena_adaptor& other) : arena(other.arena) {}

        byte_arena_adaptor& operator=(const byte_arena_adaptor& other) {
            if (this != &other) {
                arena = other.arena;
            }
            return *this;
        }

        byte_arena_adaptor(byte_arena_adaptor&& other) noexcept : arena(other.arena) {}

        byte_arena_adaptor& operator=(byte_arena_adaptor&& other) noexcept {
            if (this != &other) {
                arena = other.arena;
            }
            return *this;
        }

        template <typename U>
        bool operator == (const byte_arena_adaptor<U>& other) const {
            return arena == other.arena;
        }

        template <typename U>
        bool operator != (const byte_arena_adaptor<U>& other) const {
            return arena != other.arena;
        }

        template <typename... Args>
        T* construct(T* ptr, Args&&... args) {
            return new (ptr) T(std::forward<Args>(args)...);
        }

        void destroy(T* ptr) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
        }

        template <typename T, typename... Args>
        void construct(const type_info* type, void* mem, Args&&... args) {
            new (mem) T(std::forward<Args>(args)...);
        }

        void destroy(const type_info* type, void* mem, size_t count) {
            destroy_at(type, mem, count);
        }

        T* allocate(const size_t count) {
            cexpr::require(arena);
            
            return static_cast<T*>(arena->allocate(type_info::of<T>(), count));
        }

        void deallocate(T* ptr, const size_t count) {

        }

        void deallocate(const type_info* type, void* mem, size_t count) {
        }

        T* shrink_alloc(T* ptr, size_t, size_t) {
            return ptr;
        }

        void* allocate(const type_info* type, size_t count) {
            return arena->allocate(type, count);
        }


        void copy_construct(const type_info* type, void* dst, const void* src, size_t count) {
            copy(type, dst, src, count);
        }

        void move_construct(const type_info* type, void* dst, void* src, size_t count) {
            move(type, dst, src, count);
        }
    };

    template <typename T, typename ByteArena>
    requires requires {typename std::decay_t<ByteArena>::IsByteArena; }
    auto make_byte_arena_adaptor(ByteArena& arena) {
        return byte_arena_adaptor<T, std::decay_t<ByteArena>>{&arena};
    }

    template <typename T>
    struct allocator_traits<byte_arena_adaptor<T>> : default_allocator_traits {
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::false_type;
    };
}