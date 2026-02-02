#pragma once
#include <Math/Shapes/geom.h>
#include <Renderer/Scene/Primitives/IPrimitive.h>
#include <Renderer/Scene/Primitives/Primitive.h>
#include <Renderer/Scene/Primitives/WorldCullCallback.h>
#include <Renderer/Common/Frustum.h>

class MipWorld : public IPrimitiveWorld<MipWorld> {
public:
    struct CreateInfo {
        glm::ivec3 position = glm::ivec3(0);
        int lowestMipSize = 16;
        int highestMipSize = 512;
        bool isFinite = true;

        static bool isPowerOfTwo(uint32_t N) {
            return N != 0 && (N & (N - 1)) == 0;
        }

        int getMipCount() const {
            if (!isPowerOfTwo(lowestMipSize) || !isPowerOfTwo(highestMipSize)) {
                return -1;
            }
            return 1 + static_cast<int>(std::floor(std::log2(highestMipSize / lowestMipSize)));
        }
    };

    class Region {
    public:
        struct MipPosition {
            int mip = 0;
            int index = 0;

            bool operator == (const MipPosition& other) const {
                return mip == other.mip && index == other.index;
            }

            bool operator != (const MipPosition& other) const {
                return !(*this == other);
            }
        };

        struct EntryPosition {
            MipPosition mip{};
            unsigned regionIndex = 0;
        };

        class LocationHandle {
            friend class Region;

            unsigned index = 0;

            explicit LocationHandle(unsigned index) : index(index) {}
        public:
            LocationHandle() = default;

            unsigned id() const { return index; }

            bool operator == (const LocationHandle& other) const {
                return index == other.index;
            }

            bool operator != (const LocationHandle& other) const {
                return !(*this == other);
            }
        };
    private:
        struct MipLevel {
            std::vector<LocationHandle> localEntities{};

            unsigned next() const {
                return localEntities.size();
            }

            LocationHandle& back() {
                return localEntities.back();
            }

            void eraseLast() {
                localEntities.pop_back();
            }

            void add(const LocationHandle handle) {
                localEntities.push_back(handle);
            }

            LocationHandle& operator [] (const unsigned index) {
                return localEntities[index];
            }

            auto begin() const {
                return localEntities.begin();
            }

            auto end() const {
                return localEntities.end();
            }
        };

        struct Levels {
            std::vector<std::vector<MipLevel>> myLevels;

            MipLevel& operator [] (const MipPosition& mPosition) {
                return myLevels[mPosition.mip][mPosition.index];
            }

            void resize(const unsigned size) {
                myLevels.resize(size);
            }

            std::vector<MipLevel>& operator [] (const unsigned mipIndex) {
                return myLevels[mipIndex];
            }

            auto begin() const {
                return myLevels.begin();
            }

            auto end() const {
                return myLevels.end();
            }
        };

        struct Entities {
            std::vector<EntryPosition> myEntities;
            std::vector<LocationHandle> myFreeHandles{};

            Entities();

            LocationHandle add(const EntryPosition& ePosition);

            EntryPosition& operator [] (const LocationHandle& handle) {
                return myEntities[handle.index];
            }

            void free(LocationHandle handle);
        };

        Levels levels;
        int maxLevels = 0;
        int maxSize = 0;

        glm::ivec3 position{};

        Entities entities;

        bool infinite = false;
    public:
        LocationHandle createEntityHandle(EntryPosition ePosition);

        Region() = default;

        explicit Region(const CreateInfo &info);

        MipPosition getMipPosition(AABB aabb) const;

        LocationHandle addEntity(const AABB& worldBox);

        void updateEntity(const LocationHandle handle, const AABB& worldBox);

        void removeEntity(LocationHandle handle);

        AABB bounds() const {
            return AABB::fromTo(position, position + maxSize);
        }

        int getAt(int mip, int x, int y, int z) const;

        template <typename Fn>
        void cull(AABB worldBox, Fn&& fn) {
            worldBox.center -= position;

            if (isInfinite() || worldBox.contains(bounds())) {
                for (auto& level : levels) {
                    for (auto& mip : level) {
                        for (auto& handle : mip) {
                            fn(handle);
                        }
                    }
                }
                return;
            }

            for (auto handle : levels[0][0]) {
                fn(handle);
            }

            for (int mip = 1; mip < maxLevels; ++mip) {
                auto& mipData = levels[mip];

                const int cellSize = maxSize >> mip;

                glm::ivec3 cellMin = geom::floorDiv3(worldBox.min(), cellSize);
                glm::ivec3 cellMax = geom::floorDiv3(worldBox.max() - glm::vec3(1e-6f), cellSize);

                cellMin = glm::max(cellMin, glm::ivec3(0));

                cellMax = glm::min(cellMax, glm::ivec3(maxSize / cellSize) - glm::ivec3(1));

                for (int z = cellMin.z; z <= cellMax.z; ++z)
                    for (int y = cellMin.y; y <= cellMax.y; ++y)
                        for (int x = cellMin.x; x <= cellMax.x; ++x)
                        {
                            const int index = getAt(mip, x, y, z);

                            if (mipData[index].localEntities.empty()) continue;

                            for (auto handle : mipData[index]) {
                                fn(handle);
                            }
                        }
            }
        }

        bool isFinite() const { return !infinite; }
        bool isInfinite() const { return infinite; }

        glm::ivec3 getPosition() const { return position; }
    };

private:
    struct RegionCollectionPair {
        Region region;
        std::vector<PrimitiveCollectionID> handles;
        std::vector<unsigned> freeHandles;
    };
    std::unordered_map<glm::ivec3, RegionCollectionPair, IVec3Hash> regions{};
    int regionHighestSize = 512;
    int regionLowestSize = 16;

    std::vector<PrimitiveCollectionID> infiniteHandles{};
    std::vector<std::pair<glm::ivec3, Region::LocationHandle>> collectionToHandle{};
public:
    MipWorld(int regionLowestSize, const int regionHighestSize) : regionHighestSize(regionHighestSize), regionLowestSize(regionLowestSize) {}

    void onAddCollection(const PrimitiveCollectionID id, const PrimitiveCollection& collection) {
        if (collection.hasFlag(PrimitiveCollectionFlags::INFINITE_BOUNDS)) {
            infiniteHandles.push_back(id);
            return;
        }
        AABB aabb = collection.getWorldBounds();

        glm::ivec3 regionCoords = geom::floorDiv3(aabb.center, regionHighestSize);

        const auto it = regions.find(regionCoords);

        RegionCollectionPair* region{};

        if (it == regions.end()) {
            region = &regions.emplace(regionCoords,
                RegionCollectionPair(
                    Region(CreateInfo(regionCoords * regionHighestSize, regionLowestSize, regionHighestSize, true))
                )
            ).first->second;
        } else {
            region = &it->second;
        }

        auto handle = region->region.addEntity(collection.getWorldBounds());

        if (region->handles.size() <= handle.id()) {
            region->handles.resize(handle.id() + 64);
        }

        region->handles[handle.id()] = id;

        if (collectionToHandle.size() <= id.id()) {
            collectionToHandle.resize(id.id() + 512);
        }

        collectionToHandle[id.id()] = { regionCoords, handle };
    }

    void onUpdateCollection(const PrimitiveCollectionID id, const PrimitiveCollection& collection, const PrimitiveCollectionChange &change) {
        if (change.isTransformChanged()) {

        }
    }

    void onRemoveCollection(const PrimitiveCollectionID id, const PrimitiveCollection& collection) {
        if (collection.hasFlag(PrimitiveCollectionFlags::INFINITE_BOUNDS)) {
            auto it = std::find(infiniteHandles.begin(), infiniteHandles.end(), id);
            infiniteHandles.erase(it);
            return;
        }

        auto [regionCoords, spatialHandle] = collectionToHandle[id.id()];

        auto regionIt = regions.find(regionCoords);

        auto& [region, handles, freelist] = regionIt->second;

        region.removeEntity(spatialHandle);
        freelist.push_back(spatialHandle.id());
    }

    void onCull(const Frustum& frustum, const WorldCullCallback callback) {
        AABB frustumAABB = frustum.asAABB();

        glm::ivec3 min = geom::floorDiv3(frustumAABB.min(), regionHighestSize);
        glm::ivec3 max = geom::floorDiv3(frustumAABB.max() - glm::vec3(1e-6f), regionHighestSize);

        for (auto inf : infiniteHandles) {
            callback(inf);
        }

        for (int z = min.z; z <= max.z; ++z) {
            for (int y = min.y; y <= max.y; ++y) {
                for (int x = min.x; x <= max.x; ++x) {
                    const auto regionIt = regions.find(glm::ivec3(x, y, z));

                    if (regionIt == regions.end()) {
                        continue;
                    }

                    AABB regionAABB = regionIt->second.region.bounds();

                    if (!frustum.isAABBInsideFrustum(regionAABB)) {
                        continue;
                    }

                    auto& [region, handles, freelist] = regionIt->second;

                    region.cull(frustumAABB, [&](const Region::LocationHandle& handle) {
                        auto collectionID = handles[handle.id()];

                        callback(collectionID);
                    });
                }
            }
        }
    }
};
