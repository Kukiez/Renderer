#pragma once
#include <ECS/Component/Component.h>
#include <ECS/Component/SingletonTypeRegistry.h>

#include "RendererAPI.h"

class GraphicsPassInvocation;
class Renderer;
class RenderInvocation;

template <typename Pass>
concept IsRenderingPass = true;

class RenderingPassType {
    struct VTable {
        std::string_view name;
        template <typename T>
        static constexpr VTable of() {
            VTable vt{};
            vt.name = cexpr::name_of<T>;
            return vt;
        }
    };

    struct Ctx {
        using TypeID = RenderingPassType;
        using TypeMetadata = VTable;

        std::unordered_map<std::string_view, TypeID> nameToPass;
        std::vector<VTable> passToTable;
        tbb::concurrent_vector<std::pair<VTable, TypeID>> nameToPassPending;

        template <IsRenderingPass Pass>
        TypeMetadata onCreateMetadata(TypeID typeID) {
            nameToPassPending.emplace_back(mem::type_info_of<Pass>.name(), typeID);
            return VTable::of<Pass>();
        }

        RENDERERAPI void onFlush();

        RENDERERAPI TypeID findPassByName(const std::string_view name);
    };
    RENDERERAPI static ecs::TypeContext<Ctx> ctx;

    unsigned myID{};
public:
    struct RenderingPassComponentType {};

    constexpr RenderingPassType() = default;
    constexpr RenderingPassType(unsigned id) : myID(id) {}

    unsigned id() const { return myID; }

    bool operator==(const RenderingPassType& other) const { return myID == other.myID; }
    bool operator!=(const RenderingPassType& other) const { return myID != other.myID; }

    template <IsRenderingPass Pass>
    static RenderingPassType of() {
        return ctx.registerType<Pass>();
    }

    static RenderingPassType of(std::string_view name) {
        return ctx.findPassByName(name);
    }

    static const VTable& getVTable(const RenderingPassType& type) {
        return ctx.getTypeMetadata(type);
    }

    const VTable& getVTable() const {
        return getVTable(*this);
    }

    std::string_view name() const { return getVTable().name; }
};

static constexpr auto NULL_RENDERING_PASS_KEY = RenderingPassType{0};