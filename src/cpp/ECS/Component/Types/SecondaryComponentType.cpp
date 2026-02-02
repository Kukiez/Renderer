#include "SecondaryComponentType.h"
#include "ECS/Level/LevelContext.h"

void SecondaryComponentType::onSynchronize(LevelContext &level) {
    if (level.registry.getEntityLimit() > metadata.getCapacity()) {
        metadata.expand(level.registry.getEntityLimit(), &metadataAllocator);
    }

    for (auto& tlOps : staging) {
        auto& componentBuffer = tlOps.removedComponents;
        for (auto [type, entities] : componentBuffer.iterator(Kind)) {
            if (entities.empty()) continue;

            storage.remove(entities.data(), type, entities.size());
            entities.clear();
        }
    }
    for (auto& tlOps : staging) {
        auto& componentBuffer = tlOps.newComponents;
        for (auto [type, buffer] : componentBuffer.iterator(Kind)) {
            auto& [typeInfo, ptr, size, capacity, entities] = buffer;

            if (entities.empty()) continue;
            storage.add(entities.data(), type, ptr, entities.size());
        }
    }

    for (auto deleted : level.registry.getDeletedEntities()) {
        storage.deleteEntity(deleted);
    }
    staging.reset();
}

detail::SecondaryQueryData::SecondaryQueryData(LevelContext &context) : storage(context.registry.getComponentType<SecondaryComponentType>()) {
}
