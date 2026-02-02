#include "MipWorld.h"
#include <glm/gtx/component_wise.hpp>
#include <Math/Shapes/geom.h>

MipWorld::Region::Entities::Entities() {
    myEntities.emplace_back();
}

MipWorld::Region::LocationHandle MipWorld::Region::Entities::add(const EntryPosition &ePosition) {
    if (!myFreeHandles.empty()) {
        auto handle = myFreeHandles.back();
        myFreeHandles.pop_back();
        myEntities[handle.index] = {ePosition};
        return handle;
    }
    myEntities.emplace_back(ePosition);
    return LocationHandle{static_cast<unsigned>(myEntities.size() - 1)};
}

void MipWorld::Region::Entities::free(const LocationHandle handle) {
    myFreeHandles.push_back(handle);
}

MipWorld::Region::LocationHandle MipWorld::Region::createEntityHandle(EntryPosition ePosition) {
    return entities.add(ePosition);
}

MipWorld::Region::Region(const CreateInfo &info): infinite(!info.isFinite), position(info.position), maxLevels(info.getMipCount()), maxSize(info.highestMipSize) {
    if (isInfinite()) {
        levels.resize(1);
        levels[0].resize(1);
        return;
    }
    assert(info.getMipCount() != -1);

    levels.resize(maxLevels);

    int entries = 1;

    int size = maxSize;

    for (int lvl = 0; lvl < maxLevels; ++lvl) {
        auto& level = levels[lvl];

        level.resize(entries);

        std::cout << "Mip: " << lvl << " Entries: " << entries << ", Size: " << size << std::endl;
        entries *= 8;
        size /= 2;
    }
}

MipWorld::Region::MipPosition MipWorld::Region::getMipPosition(AABB aabb) const {
    if (isInfinite())
        return {};

    aabb.center -= position;
    const glm::vec3 extents = aabb.halfSize * 2.0f;
    const float maxExtent = glm::compMax(extents);

    const float safeExtent = glm::max(maxExtent, 1e-6f);
    int mip = static_cast<int>(glm::floor(glm::log2(static_cast<float>(maxSize) / safeExtent)));
    mip = glm::clamp(mip, 0, maxLevels - 1);

    int mipSize = maxSize >> mip;

    glm::ivec3 cellMin =
        geom::floorDiv3(aabb.min(), mipSize);

    glm::ivec3 cellMax =
        geom::floorDiv3(aabb.max() - glm::vec3(1e-6f), mipSize);

    cellMin = glm::max(cellMin, glm::ivec3(0));
    cellMax = glm::min(cellMax, glm::ivec3(maxSize / mipSize) - glm::ivec3(1));

    if (cellMin != cellMax && mip > 0) {
        --mip;
        mipSize = maxSize >> mip;
        cellMin = geom::floorDiv3(aabb.min(), mipSize);
        cellMin = glm::max(cellMin, glm::ivec3(0));
    }

    const glm::ivec3 cell = cellMin;
    const int index = getAt(mip, cell.x, cell.y, cell.z);

    return MipPosition{ mip, index };
}

MipWorld::Region::LocationHandle MipWorld::Region::addEntity(const AABB& worldBox) {
    auto mipPos = getMipPosition(worldBox);
    auto& mipData = levels[mipPos];

    auto h = createEntityHandle({mipPos, mipData.next()});
    levels[mipPos].add(h);
    return h;
}

void MipWorld::Region::updateEntity(const LocationHandle handle, const AABB& worldBox) {
    assert(handle.id() != 0);

    auto newMipPosition = getMipPosition(worldBox);

    auto& liveEntityPosition = entities[handle];

    if (liveEntityPosition.mip != newMipPosition) {
        auto& oldMipLevel = levels[liveEntityPosition.mip];
        auto& newMipLevel = levels[newMipPosition];

        auto& oldMipLastHandle = oldMipLevel.back();

        if (handle == oldMipLastHandle) {
            oldMipLevel.eraseLast();
        } else {
            auto& oldMipLastEntityPosition = entities[oldMipLastHandle];

            oldMipLastEntityPosition = liveEntityPosition;

            oldMipLevel[oldMipLastEntityPosition.regionIndex] = oldMipLastHandle;

            oldMipLevel.eraseLast();
        }

        auto next = newMipLevel.next();
        newMipLevel.add(handle);
        liveEntityPosition.mip = newMipPosition;
        liveEntityPosition.regionIndex = next;
    }
}

void MipWorld::Region::removeEntity(const LocationHandle handle) {
    assert(handle.id() != 0);

    auto& liveEntityPosition = entities[handle];

    auto& mipData = levels[liveEntityPosition.mip];

    auto& mipLastEntityHandle = mipData.back();

    if (handle != mipLastEntityHandle) {
        auto& lastEntityPosition = entities[mipLastEntityHandle];
        lastEntityPosition = liveEntityPosition;
        mipData[lastEntityPosition.regionIndex] = mipLastEntityHandle;
    }
    liveEntityPosition = {};
    mipData.eraseLast();

    entities.free(handle);
}

int MipWorld::Region::getAt(const int mip, const int x, const int y, const int z) const {
    const int mipSize = maxSize >> mip;

    int numRegions = maxSize / mipSize;
    return x + y * numRegions + z * numRegions * numRegions;
}
