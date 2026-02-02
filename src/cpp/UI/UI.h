#pragma once
#include <ECS/Component/Component.h>
#include <glm/vec4.hpp>
#include <memory/Span.h>
#include <util/enum_bit.h>
#include <Renderer/Common/Interpolation.h>

#include "UI/UITypeContext.h"
#include "UITransform.h"
#include "UIAnimation.h"

namespace ui {
    class UIWindowObject;
}

namespace ui {
    class UIObject;

    enum class UIOptions {
        NONE = 0,
        DRAGGABLE = 1 << 0,
        DROP_TARGETABLE = 1 << 1,
        CLAMP_TO_PARENT = 1 << 2
    };

    enum class UIStateFlags {
        NONE = 0,

    };

    using UIOptionFlags = EnumFlags<UIOptions>;

    enum class UITransformOptions {
        RELATIVE_TO_PARENT,
        RELATIVE_TO_SCREEN,
        FIXED,
        CENTERED
    };

    using UITransformFlags = EnumFlags<UITransformOptions>;

    struct NoLayout {};

    struct UILayout {
        void* layoutObj{};
        UIObjectTypePtr type{};

        UITransform transform{};
    };

    struct UIRectangleShape : UIBounds {};

    struct UIShape {
        enum class ShapeType {
            RECTANGLE = 0
        };

        union {
            UIRectangleShape rect{};
        };
        ShapeType type = ShapeType::RECTANGLE;

        UIShape() = default;
        UIShape(const UIRectangleShape rect) : rect(rect), type(ShapeType::RECTANGLE) {}
    };

    template <typename TConcrete, typename TShape = UIRectangleShape, typename TLayout = NoLayout>
    struct UIObjectDesc {
        TConcrete data{};
        TShape shape{};
        TLayout layout{};
        UIOptionFlags options{};
        UITransformFlags transformFlags{};
        UITransform localTransform{};
        mem::range<UIObject> children{};
    };

    class UIRuntimeObject {
    public:
        UIRuntimeObject* parent = nullptr;
        UIRuntimeObject** children = nullptr;
        size_t numChildren = 0;

        UILayout layout{};

        UITransform finalTransform;

        UITransformAnimationState* animationState{};

        UIWindowObject* window{};

        UIObjectTypePtr objType{};
        void* object{};

        UIShape shape{};

        struct Constructor {
            UIWindowObject* window{};
            UIObjectTypePtr objType{};
            void* objData{};

            UILayout layout{};
            UIShape shape{};

            UITransform localTransform{};

            UIOptionFlags options = UIOptions::NONE;
            UITransformFlags transformFlags = UITransformOptions::RELATIVE_TO_PARENT;
            glm::vec2 dragDeltaRequired = glm::vec2(0.1f);

            UIRuntimeObject** children{};
            size_t numChildren{};
        };

        using ComponentType = UIObjectComponentType;

        UIOptionFlags options = UIOptions::NONE;
        UITransformFlags transformFlags = UITransformOptions::RELATIVE_TO_PARENT;
        glm::vec2 dragDeltaRequired = glm::vec2(0.1f);

        UITransform localTransform{};
        UIBounds bounds{};

        bool visible = true;

        UIRuntimeObject() = default;

        explicit UIRuntimeObject(const Constructor& ctr) {
            options = ctr.options;
            transformFlags = ctr.transformFlags;
            dragDeltaRequired = ctr.dragDeltaRequired;
            localTransform = ctr.localTransform;
            window = ctr.window;
            objType = ctr.objType;
            object = ctr.objData;
            layout = ctr.layout;
            shape = ctr.shape;
            children = ctr.children;
            numChildren = ctr.numChildren;
        }

        bool isDraggable() const { return options.has(UIOptions::DRAGGABLE); }

        bool isDropTargetable() const { return options.has(UIOptions::DROP_TARGETABLE); }

        auto getChildren() const { return mem::make_range(children, numChildren); }

        UIObjectTypePtr getType() const { return objType; }

        UILayout getLayout() const { return layout; }

        bool hasLayout() const { return layout.layoutObj != nullptr; }

        const UITransform& getLocalTransform() const { return localTransform; }
        const UITransform& getFinalTransform() const { return finalTransform; }
        const UITransform& getLayoutTransform() const { return layout.transform; }

        bool hasAnimationState() const { return animationState != nullptr; }
        const UITransformAnimationState& getAnimationState() const { return *animationState; }

        UIRuntimeObject* getParent() const { return parent; }

        template <typename T>
        T* is() {
            auto tType = UITypes.getTypeID<T>();

            if (tType == objType) {
                return static_cast<T*>(object);
            }
            return nullptr;
        }
    };

    struct UIObjectBase {

    };

    class UIObject {
        template <typename T>
        friend class TUIObject;

        UIRuntimeObject* object{};
    public:
        UIObject() = default;
        UIObject(UIRuntimeObject* object) : object(object) {}

        UIObjectTypePtr getType() const { return object->getType(); }
        UIObjectTypePtr getLayoutType() const { return object->getLayout().type; }

        const UIWindowObject* getWindow() const { return object->window; }

        bool isDraggable() const { return object->isDraggable(); }
        bool isDropTargetable() const { return object->isDropTargetable(); }

        glm::vec2 getDragDeltaRequired() const { return object->dragDeltaRequired; }


        UIObject getParent() const { return UIObject(object->getParent()); }


        const UITransform& getLocalTransform() const { return object->getLocalTransform(); }
        const UITransform& getFinalTransform() const { return object->getFinalTransform(); }

        operator bool() const { return object != nullptr; }

        bool operator != (const UIObject& other) const { return object != other.object; }
        bool operator == (const UIObject& other) const { return object == other.object; }

        template <typename T>
        TUIObject<T> is();

        /* internal */
        UIRuntimeObject* getInternalObject() const { return object; }
    };

    template <typename T>
    class TUIObject : public UIObject {
    public:
        using UIObject::UIObject;

        T* operator->() { return static_cast<T*>(object->object); }
        const T* operator->() const { return static_cast<const T*>(object->object); }

        operator T*() { return static_cast<T*>(object->object); }
    };

    template <typename T>
    TUIObject<T> UIObject::is() {
        if (auto tObject = object->is<T>()) {
            return TUIObject<T>(*this);
        }
        return TUIObject<T>();
    }
}
