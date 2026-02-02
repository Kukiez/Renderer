#pragma once
#include <ECS/System/SystemKindRegistry.h>
#include <ECS/Component/SingletonTypeRegistry.h>
#include <UI/UI.h>

namespace ui {
    struct UIStyleMetadata {
        ComponentIndex renderer{};
    };

    class StyleType {
        unsigned typeIndex = 0;
    public:
        StyleType() = default;
        explicit StyleType(unsigned typeIndex) : typeIndex(typeIndex) {}

        unsigned id() const { return typeIndex; }

        bool operator==(const StyleType& other) const { return typeIndex == other.typeIndex; }
        bool operator != (const StyleType& other) const { return typeIndex != other.typeIndex; }
    };

    struct UIStyleType : ComponentType<UIStyleType> {
        using TypeID = StyleType;
        using TypeMetadata = UIStyleMetadata;

        template <typename T>
        static TypeMetadata onCreateMetadata(TypeID id) {
            return UIStyleMetadata{};
        }

        template <typename Style, typename Renderer>
        void setRenderer(this auto& ctx) {
            auto& style = const_cast<UIStyleMetadata&>(ctx.template getTypeMetadata<Style>());

            style.renderer = ComponentIndexValue<SystemComponentType, Renderer>::value;
        }

        ComponentIndex getRenderer(this auto& ctx, TypeID style) {
            return ctx.getTypeMetadata(style).renderer;
        }
    };

    extern ecs::TypeContext<UIStyleType> UIStyleTypes;

    template <typename T>
    concept IsUIStyle = true;

    template <typename TStyle>
    struct UIStyleDrawCommand {
        TStyle style{};
        UITransform transform{};
    };

    class UIDrawCommand {
        friend class UIRenderingContext;

        void* styleData{};
        UITransform transform{};

        UIStyleType::TypeID styleType{};
    public:
        const UITransform& getTransform() const { return transform; }

        template <IsUIStyle T>
        const T* is() const {
            if (UIStyleTypes.of<T>() == styleType) {
                return static_cast<T*>(styleData);
            }
            return nullptr;
        }

        UIStyleType::TypeID getStyleType() const { return styleType; }
    };
}
