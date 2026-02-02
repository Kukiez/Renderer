#include "NameComponentType.h"

void NameStagingBuffer::EntityNameChangeOps::add(Entity entity, const EntityName& name) {
    auto* mem = static_cast<char*>(allocator.allocate(mem::type_info_of<char>, name.length() + 1));
    memcpy(mem, name.data(), name.length());
    mem[name.length()] = '\0';
    nameChanges.emplace_back(entity, EntityName(mem, name.length(), name.hash()));
}

void NameStagingBuffer::setName(const Entity entity, const EntityName &name) {
    nameChangeOps.local().add(entity, name);
}

void NameStagingBuffer::reset() {
    for (auto& [arena, names] : nameChangeOps) {
        names = NameVector(&arena);
        arena.reset_compact();
    }
}

auto NameStagingBuffer::begin() {
    return nameChangeOps.begin();
}

auto NameStagingBuffer::end() {
    return nameChangeOps.end();
}


void NameComponentType::onSynchronize(LevelContext &level) {
    if (level.registry.getEntityLimit() > metadata.getCapacity()) {
        metadata.expand(level.registry.getEntityLimit());
    }

    for (auto& tlOps : staging) {
        auto& namesBuffer = tlOps.nameChanges;

        for (auto& [entity, name] : namesBuffer) {
            EntityName* entityName;

            if (const auto it = names.find(name.hash()); it != names.end()) {
                entityName = &it->second.name;
                it->second.refCount++;
            } else {
                char* mem = allocator.allocate(mem::type_info_of<char>, name.size() + 1);
                memcpy(mem, name.data(), name.size());
                mem[name.size()] = '\0';

                entityName = &names.emplace(std::piecewise_construct,
                    std::forward_as_tuple(name.hash()),
                    std::forward_as_tuple(EntityName(mem, name.size(), name.hash()), 1)
                ).first->second.name;
            }

            metadata[entity] = *entityName;
        }
    }

    for (auto deleted : level.registry.getDeletedEntities()) {
        auto hash = metadata[deleted].hash();

        const auto it = names.find(hash);
        auto& [name, refCount] = it->second;

        refCount--;

        if (refCount == 0) {
            allocator.deallocate((void*)name.data(), name.length() + 1);
            names.erase(it);
        }

        metadata[deleted] = EntityName();
    }
    staging.reset();
}

NameQuery::NameQuery(LevelContext &level) : storage(level.registry.getComponentType<NameComponentType>()) {
}

template<typename T>
void NameComponentType::onRemoveComponent(const Entity &e) {
    staging.setName(e, EntityName());
}

template void NameComponentType::onRemoveComponent<EntityName>(const Entity&);