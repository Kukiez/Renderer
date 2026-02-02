#pragma once
#include <memory/byte_arena.h>
#include <memory/byte_stream.h>

namespace ui {
    class UIWindowAllocationContext {
        using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;

        Arena arena;
    public:
        UIWindowAllocationContext() : arena(16 * 1024) {
        }

        Arena& getArena() { return arena; }

        void* allocate(const UIObjectTypePtr object) {
            auto& metadata = UITypes.getTypeMetadata(object);
            return arena.allocate(metadata.type, 1);
        }

        void* allocate(mem::typeindex type, size_t count = 1) {
            return arena.allocate(type, count);
        }

        template <typename T>
        T* allocate(size_t count = 1) {
            return static_cast<T*>(allocate(mem::type_info_of<T>, count));
        }

        void deallocate(void* adr, mem::typeindex type, size_t count = 1) {
            // TODO deallocate memory
        }
    };
};