#pragma once
#include <ECS/Entity/Entity.h>

template <typename... Sorted>
class EntityCreateBuffer {
    using SortedTuple = std::tuple<Sorted...>;

    template <typename T>
    size_t index_of() {
        static constexpr auto SortedIndex = cexpr::find_tuple_typename_index_v<std::decay_t<T>, SortedTuple>;
        const size_t index = indices[SortedIndex];
        return index;
    }

    EntityCommandBuffer<CreateTag>* buffer{};
    const int* indices;
public:
    explicit EntityCreateBuffer(auto* buffer, const int* indices) : buffer(buffer), indices(indices) {}

    template <typename... Cs>
    requires (sizeof...(Cs) == sizeof...(Sorted))
    auto createEntity(const Entity& entity, Cs&&... cs) {
        const size_t currentIndex = buffer->current();

        ([&]{
            using PtrType = std::decay_t<Cs>;
            const auto TypeIndex = index_of<PtrType>();
            PtrType* typeArray = static_cast<PtrType*>(buffer->data[TypeIndex]);
            PtrType* ptr = &typeArray[currentIndex];
            new (ptr) PtrType(std::forward<Cs>(cs));

            std::cout << "Writing Adr[" << ptr << "]: " << cexpr::name_of<PtrType> << ", Bytes: " << sizeof(PtrType) << std::endl;
        }(), ...);
        new (&buffer->entities[buffer->current()]) Entity(entity);
        ++buffer->size;

        return std::make_tuple(
            &static_cast<std::decay_t<Cs>*>(
                buffer->data[index_of<std::decay_t<Cs>>()]
            )[currentIndex]
            ...
        );
    }
};