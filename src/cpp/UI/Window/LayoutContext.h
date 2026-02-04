#pragma once
#include <unordered_set>
#include <UI/UI.h>
#include <UI/UITypeContext.h>
#include "UI/UICursor.h"

struct MouseButtonEvent;
struct MouseMotionEvent;
struct MouseState;

namespace ui {
    class UIWindowLayoutContext {
        static glm::vec2 clampChildToParent(
            const UIBounds& parent,
            const UIBounds& child);

        static bool clampChildTransform(UIBounds parentBounds, UIRuntimeObject* child);

        void updateChildrenTransforms(mem::range<UIRuntimeObject> children);

        void updateUIObject(const UIRuntimeObject* parent, UIRuntimeObject* child);

        void updateNodeChildrenTransforms(UIRuntimeObject* parent);

        UIWindowObject* rootObj{};
        std::unordered_set<UIRuntimeObject*> dirtyObjects;
    public:
        UIWindowLayoutContext() = default;
        UIWindowLayoutContext(UIWindowObject* root) : rootObj(root) {}

        UIAPI void updateLayout();

        UIAPI void addDirty(UIRuntimeObject* obj);

        auto& getRoot() const { return rootObj; }
        auto& getRoot() { return rootObj; }

        UIRuntimeObject* getObject(UIRuntimeObject* parent, glm::vec2 position) const;
        UIAPI UIRuntimeObject* getObject(glm::vec2 position) const;

        UIAPI void handleDragMotionEvent(UICursor& cursor, const MouseState &mouse, const MouseMotionEvent &event);

        UIAPI void handleDragEndEvent(UICursor &cursor, const MouseState &mouse, const MouseButtonEvent &event);

        UIAPI void handleClickEvent(UICursor &cursor, const MouseState& mouse, const MouseButtonEvent &event);

        UIAPI void handleMotionEvent(UICursor &cursor, const MouseState& mouse, const MouseMotionEvent &event);

        UIAPI void handleWheelEvent(UICursor &cursor, const MouseState& mouse, const MouseMotionEvent &event) const;
    };
}
