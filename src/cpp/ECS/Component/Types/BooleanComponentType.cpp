//
// Created by dkuki on 11/6/2025.
//

#include "BooleanComponentType.h"

#include "ECS/Level/LevelContext.h"

void BooleanComponentType::onSynchronize(LevelContext &level) {
    if (level.registry.getEntityLimit() > metadata.getCapacity()) {
        metadata.expand(level.registry.getEntityLimit(), &metadataAllocator);
    }

    for (auto& tlOps : staging) {
        auto& booleans = tlOps.changes;
        for (auto [type, entities] : booleans.iterator(Kind)) {
            auto& [newEntities, removedEntities] = entities;

            if (!newEntities.empty()) {
                storage.add(newEntities.data(), type, newEntities.size());
            }
            if (!removedEntities.empty()) {
                storage.remove(removedEntities.data(), type, removedEntities.size());
            }
        }
    }

    for (auto deleted : level.registry.getDeletedEntities()) {
        storage.deleteEntity(deleted);
    }
    staging.reset();
}

detail::BooleanQueryData::BooleanQueryData(LevelContext &context) : storage(context.registry.getComponentType<BooleanComponentType>()) {
}
