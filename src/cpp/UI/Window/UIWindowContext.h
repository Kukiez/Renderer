#pragma once
#include <algorithm>
#include <ranges>
#include <ECS/Component/ComponentTypeRegistry.h>
#include <ECS/Entity/Entity.h>
#include <glm/vec2.hpp>
#include <oneapi/tbb/concurrent_vector.h>
#include <UI/UI.h>

#include "UI/UIContext.h"
#include "UI/UITypeContext.h"
#include "LayoutContext.h"
#include "AllocationContext.h"
#include "UIAnimationContext.h"

namespace ui {
    static constexpr auto UI_DEBUG = true;

    class UIWindowContext {
        struct UINode {
            UIRuntimeObject* object{};
            UITransformAnimationState* animation{};
        };

        UIWindowAnimationContext animationCtx{};
        UIWindowLayoutContext layout;
        UIWindowAllocationContext allocCtx{};

        struct UIObjectArray {
            UIRuntimeObject* objects{};
            size_t numObjects = 0;
            size_t capObjects = 0;
        };

        std::vector<UIObjectArray> uiObjects{};
        std::vector<UIRuntimeObject*> freeObjects{};
    public:
        UIWindowContext(size_t reserve = 64) {
            uiObjects.emplace_back(new UIRuntimeObject[reserve], 0, reserve);
        }

        void setRoot(UIWindowObject* root) {
            layout.getRoot() = root;
        }

        template <typename TConcrete, typename TShape, typename TLayout>
        TUIObject<TConcrete> createObject(const UIObjectDesc<TConcrete, TShape, TLayout>& objDesc) {
            void* tObjectData = allocCtx.allocate(mem::type_info_of<TConcrete>);
            new (tObjectData) TConcrete(objDesc.data);

            void* tLayoutData = nullptr;

            if constexpr (!std::is_same_v<TLayout, NoLayout>) {
                tLayoutData = allocCtx.allocate(mem::type_info_of<TLayout>);
                new (tLayoutData) TLayout(objDesc.layout);
            }

            UIRuntimeObject** childrenArr = nullptr;
            size_t numChildren = 0;

            if (!objDesc.children.empty()) {
                childrenArr = allocCtx.allocate<UIRuntimeObject*>(objDesc.children.size());
                std::memcpy(childrenArr, objDesc.children.data(), objDesc.children.size() * sizeof(UIRuntimeObject*));
                numChildren = objDesc.children.size();
            }

            const UIRuntimeObject::Constructor ctr = {
                .window = layout.getRoot(),
                .objType = UITypes.registerType<TConcrete>(),
                .objData = tObjectData,
                .layout = UILayout(tLayoutData, UITypes.registerType<TLayout>()),
                .shape = createShape(objDesc.shape),
                .localTransform = objDesc.localTransform,
                .options = objDesc.options,
                .transformFlags = objDesc.transformFlags,
                .dragDeltaRequired = glm::vec2(0.1f),
                .children = childrenArr,
                .numChildren = numChildren
            };

            auto objData = findObjectSlot();
            new (objData) UIRuntimeObject(ctr);

            return {objData};
        }

        UIAPI void addObject(UIObject obj);

        UIAPI void destroyObject(UIObject obj);

        UIAPI void render(UIRenderingContext& ctx) const;

        UIAPI bool clickCursor(UICursorObject cursor, const MouseState& mouse, const MouseButtonEvent& button);

        UIAPI bool moveCursor(UICursorObject cursor, const MouseState& mouse, const MouseMotionEvent& motion);

        UIAPI bool moveCursorWheel(UICursorObject cursor, const MouseState& mouse, const MouseMotionEvent& wheel);

        void updateLayout() {
            layout.updateLayout();
        }

        void advanceAnimations(const double deltaTime) {
            animationCtx.advanceTransformAnimations(deltaTime);
        }
    private:
        void detachObjectImpl(UIRuntimeObject**& children, size_t& numChildren, const UIRuntimeObject* child, size_t hint = -1);

        void detachObject(UIObject child, size_t hint = -1);

        static size_t findChildIndex(UIRuntimeObject** children, size_t numChildren, const UIRuntimeObject* child);

        void setObjectParentImpl(UIRuntimeObject**& parentChildren, size_t& numChildren, UIRuntimeObject* object);

        void setObjectParent(UIObject parent, UIObject child);

        void addRootObject(UIObject obj);

        UIRuntimeObject* findObjectSlot();

        template <typename Shape>
        UIShape createShape(Shape&& shapeData) {
            using TShape = std::decay_t<Shape>;

            if constexpr (std::is_same_v<TShape, UIRectangleShape>) {
                return UIShape(shapeData);
            } else {
                static_assert(false, "not implemented");
            }
        }
    };

    struct UIWindowDesc {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::string_view name;
    };

    class UIWindowObject {
        friend class UIWindowContext;
        friend class UIWindowLayoutContext;

        glm::vec2 position{};
        glm::vec2 size{};

        UIRuntimeObject** children{};
        size_t numChildren{};

        std::string name{};

        UIWindowContext window{};
    public:
        UIWindowObject(const UIWindowDesc& desc) : position(desc.x, desc.y), size(desc.width, desc.height), name(desc.name), window(64) {
            window.setRoot(this);
        }

        template <typename TObject, typename TShape, typename TLayout>
        TUIObject<TObject> createObject(const UIObjectDesc<TObject, TShape, TLayout>& objDesc) {
            return window.createObject(objDesc);
        }

        void addObject(const UIObject obj) {
            window.addObject(obj);
        }

        void destroyObject(const UIObject obj) {
            window.destroyObject(obj);
        }

        UIBounds bounds() const {
            return {glm::vec2(position), glm::vec2(position + size)};
        }

        glm::vec2 getPosition() const { return position; }
        glm::vec2 getSize() const { return size; }

        UITransform getTransform() const { return {position, size}; }

        auto getChildren() const { return mem::range(children, numChildren); }

        std::string_view getName() const { return name; }

        void render(UIRenderingContext& ctx) const { window.render(ctx); }

        void updateLayout() { window.updateLayout(); }
        void advanceAnimations(const double deltaTime) { window.advanceAnimations(deltaTime); }


        void click(UICursorObject cursor, const MouseState& mouse, const MouseButtonEvent& button) {
            window.clickCursor(cursor, mouse, button);
        }

        void wheel(UICursorObject cursor, const MouseState& mouse, const MouseMotionEvent& wheel) {
            window.moveCursorWheel(cursor, mouse, wheel);
        }
    };
}
