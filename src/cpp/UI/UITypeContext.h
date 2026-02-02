#pragma once
#include <ranges>
#include <vector>
#include <ECS/Component/ComponentTypeRegistry.h>
#include <glm/vec2.hpp>
#include <oneapi/tbb/concurrent_vector.h>
#include <ECS/Component/SingletonTypeRegistry.h>

namespace ui {
    class UIObject;
    class UIRenderingContext;
    struct UITypeMetadata;
    struct UIDragEvent;
    struct UIClickEvent;
    struct UIHoverEvent;
    struct UIWheelEvent;

    struct UIBounds;

    class UIObjectTypePtr {
        unsigned typeIndex{};

        bool operator==(const UIObjectTypePtr & other) const {
            return typeIndex == other.typeIndex;
        }

    public:
        UIObjectTypePtr() = default;
        UIObjectTypePtr(unsigned typeIndex) : typeIndex(typeIndex) {}

        unsigned id() const { return typeIndex; }
    };


    class UIObjectLayoutReference;

    template <typename T>
    concept HasOnComputeBounds = requires(T t) {
        t.getBounds();
    };

    template <typename T>
    concept HasOnRender = requires(T t, const UIObject& obj, UIRenderingContext& ctx) {
        t.onRender(obj, ctx);
    };

    template <typename T>
    concept HasOnComputeLayout = requires(T t, UIObjectLayoutReference& ref) {
        t.onComputeLayout(ref);
    };

    template <typename T>
    concept HasOnDrag = requires(T t, UIDragEvent& ev) {
        t.onDrag(ev);
    };

    template <typename T>
    concept HasOnClick = requires(T t, UIClickEvent& ev)
    {
        t.onClick(ev);
    };

    template <typename T>
    concept HasOnHover = requires(T t, UIHoverEvent& ev)
    {
        t.onHover(ev);
    };

    template <typename T>
    concept HasOnWheel = requires(T t, UIWheelEvent& ev)
    {
        t.onWheel(ev);
    };

    struct UITypeMetadata {
        /* UIObject HasOnRender */        using OnRenderFn = void(*)(void* object, const UIObject& obj, UIRenderingContext& ctx);
        /* UIObject HasOnDrag */          using OnDragFn = void(*)(void* object, const UIDragEvent& ev);
        /* UIObject HasOnClick */         using OnClickFn = void(*)(void* object, const UIClickEvent& ev);
        /* UIObject HasOnHover */         using OnHoverFn = void(*)(void* object, const UIHoverEvent& ev);
        /* UIObject HasOnWheel */         using OnWheelFn = void(*)(void* object, const UIWheelEvent& ev);

        /* UIShape  HasOnComputeBounds */ using OnComputeBoundsFn = UIBounds(*)(void* object);
        /* UILayout HasOnComputeLayout */ using OnComputeChildrenLayoutFn = void(*)(void* object, const UIObjectLayoutReference& ref);

        mem::typeindex type{};

        OnRenderFn onRender{};
        OnComputeBoundsFn onComputeBounds{};
        OnComputeChildrenLayoutFn onComputeChildrenLayout{};

        OnDragFn onDrag{};
        OnClickFn onClick{};
        OnHoverFn onHover{};
        OnWheelFn onWheel{};

        template <typename UIObj>
        static UITypeMetadata of() {
            UITypeMetadata meta;
            meta.type = mem::type_info_of<UIObj>;

            if constexpr (HasOnComputeBounds<UIObj>) {
                meta.onComputeBounds = [](void* object) {
                    auto* obj = static_cast<UIObj*>(object);

                    return obj->getBounds();
                };
            }

            if constexpr (HasOnComputeLayout<UIObj>) {
                meta.onComputeChildrenLayout = [](void* object, const UIObjectLayoutReference& ref) {
                    auto* obj = static_cast<UIObj*>(object);

                    obj->onComputeLayout(ref);
                };
            }

            if constexpr (HasOnDrag<UIObj>) {
                meta.onDrag = [](void* object, const UIDragEvent& ev) {
                    auto* obj = static_cast<UIObj*>(object);
                    obj->onDrag(ev);
                };
            }

            if constexpr (HasOnRender<UIObj>) {
                meta.onRender = [](void* object, const UIObject& uiObj, UIRenderingContext& ctx) {
                    auto* obj = static_cast<UIObj*>(object);

                    obj->onRender(uiObj, ctx);
                };
            }

            if constexpr (HasOnClick<UIObj>) {
                meta.onClick = [](void* object, const UIClickEvent& ev) {
                    auto* obj = static_cast<UIObj*>(object);
                    obj->onClick(ev);
                };
            }
            if constexpr (HasOnHover<UIObj>) {
                meta.onHover = [](void* object, const UIHoverEvent& ev) {
                    auto* obj = static_cast<UIObj*>(object);
                    obj->onHover(ev);
                };
            }
            if constexpr (HasOnWheel<UIObj>) {
                meta.onWheel = [](void* object, const UIWheelEvent& ev) {
                    auto* obj = static_cast<UIObj*>(object);
                    obj->onWheel(ev);
                };
            }
            return meta;
        }
    };

    struct UIObjectComponentType : ComponentType<UIObjectComponentType> {
        using TypeID = UIObjectTypePtr;
        using TypeMetadata = UITypeMetadata;

        template <typename T>
        static TypeMetadata onCreateMetadata(TypeID id) {
            return UITypeMetadata::of<T>();
        }
    };

    extern ecs::TypeContext<UIObjectComponentType> UITypes;
}
