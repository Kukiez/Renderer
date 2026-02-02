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
        UIWindowObject* rootObj{};
        std::unordered_set<UIRuntimeObject*> dirtyObjects;
    public:
        UIWindowLayoutContext() = default;
        UIWindowLayoutContext(UIWindowObject* root) : rootObj(root) {}

        static glm::vec2 clampChildToParent(
            const UIBounds& parent,
            const UIBounds& child);

        static bool clampChildTransform(UIBounds parentBounds, UIRuntimeObject* child);

        void updateChildrenTransforms(mem::range<UIRuntimeObject> children);

        void updateUIObject(const UIRuntimeObject* parent, UIRuntimeObject* child);

        void updateNodeChildrenTransforms(UIRuntimeObject* parent);

        void updateLayout();

        void addDirty(UIRuntimeObject* obj);

        auto& getRoot() const { return rootObj; }
        auto& getRoot() { return rootObj; }

        UIRuntimeObject* getObject(UIRuntimeObject* parent, glm::vec2 position) const;
        UIRuntimeObject* getObject(glm::vec2 position) const;

        void handleDragMotionEvent(UICursor& cursor, const MouseState &mouse, const MouseMotionEvent &event);

        void handleDragEndEvent(UICursor &cursor, const MouseState &mouse, const MouseButtonEvent &event);

        void handleClickEvent(UICursor &cursor, const MouseState& mouse, const MouseButtonEvent &event);

        void handleMotionEvent(UICursor &cursor, const MouseState& mouse, const MouseMotionEvent &event);

        void handleWheelEvent(UICursor &cursor, const MouseState& mouse, const MouseMotionEvent &event) const;
    };
}
