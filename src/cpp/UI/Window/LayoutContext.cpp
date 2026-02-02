#include "LayoutContext.h"

#include <UI/AppWindow.h>
#include <UI/UIEvents.h>
#include <UI/Window/UIWindowContext.h>

#include "../UIReference.h"

glm::vec2 ui::UIWindowLayoutContext::clampChildToParent(const UIBounds &parent, const UIBounds &child) {
    glm::vec2 offset(0.0f);

    // X axis
    if (child.min.x < parent.min.x)
        offset.x = parent.min.x - child.min.x;
    else if (child.max.x > parent.max.x)
        offset.x = parent.max.x - child.max.x;

    // Y axis
    if (child.min.y < parent.min.y)
        offset.y = parent.min.y - child.min.y;
    else if (child.max.y > parent.max.y)
        offset.y = parent.max.y - child.max.y;

    return offset;
}

bool ui::UIWindowLayoutContext::clampChildTransform(UIBounds parentBounds,  UIRuntimeObject *child) {
    if (!parentBounds.contains(child->bounds)) {
        const glm::vec2 overlap = clampChildToParent(parentBounds, child->bounds);

        auto& cFinal = const_cast<UITransform&>(child->getFinalTransform());
        cFinal.position += overlap;
        child->bounds.min += overlap;

        if (glm::any(glm::greaterThan(overlap, glm::vec2(0.0001f)))) {
            return true;
        }
    }
    return false;
}

void ui::UIWindowLayoutContext::updateChildrenTransforms(mem::range<UIRuntimeObject> children) {
    for (UIRuntimeObject& entity : children) {
        UITransformAnimationState* child = &const_cast<UITransformAnimationState&>(entity.getAnimationState());

        if (child->numPersistentAnimationsSnapshot != 0) {
            if (child->persistentAnimationsTotalWeight > 0.0001f) {
                child->localTransform = child->animOverrideFinalTransform / child->persistentAnimationsTotalWeight;
            }
            child->persistentAnimationsTotalWeight = 0;
            child->animOverrideFinalTransform = {};

            child->numPersistentAnimationsSnapshot = child->numPersistentAnimations;
        }
    }
}

void ui::UIWindowLayoutContext::updateUIObject(const UIRuntimeObject *parent, UIRuntimeObject *child) {
    auto& tChildFinal = child->finalTransform;

    // PERSISTENT ANIMATION Transform
    if (child->hasAnimationState()) {
        UITransformAnimationState* childAnimationState = child->animationState;
        if (childAnimationState->numPersistentAnimationsSnapshot != 0) {
            if (childAnimationState->persistentAnimationsTotalWeight > 0.0001f) {
                childAnimationState->localTransform = childAnimationState->animOverrideFinalTransform / childAnimationState->persistentAnimationsTotalWeight;
            }
            childAnimationState->persistentAnimationsTotalWeight = 0;
            childAnimationState->animOverrideFinalTransform = {};

            childAnimationState->numPersistentAnimationsSnapshot = childAnimationState->numPersistentAnimations;
        }
    }

    // LOCAL Transform
    tChildFinal.position = child->localTransform.position;
    tChildFinal.size = child->localTransform.size;

    // PARENT Transform
    tChildFinal.position += parent ? parent->getFinalTransform().position : rootObj->position;

    // LAYOUT Transform
    tChildFinal += child->getLayoutTransform();

    // FADE ANIMATION Transform
    if (child->hasAnimationState()) {
        UITransformAnimationState* childAnimationState = child->animationState;
        if (childAnimationState->numConcurrentAnimationsSnapshot != 0) {
            tChildFinal += childAnimationState->animAdditiveFinalTransform;
            childAnimationState->animAdditiveFinalTransform = {};

            childAnimationState->numConcurrentAnimationsSnapshot = childAnimationState->numConcurrentAnimations;
        }
    }

    // CLAMP Transform
    if (child->options.has(UIOptions::CLAMP_TO_PARENT)) {
        clampChildTransform(parent ? parent->bounds : rootObj->bounds(), child);
    }

    switch (child->shape.type) {
        case UIShape::ShapeType::RECTANGLE:
            child->bounds.min = tChildFinal.position;
            child->bounds.max = tChildFinal.position + tChildFinal.size;
            break;
        default: std::unreachable();
    }
}

void ui::UIWindowLayoutContext::updateNodeChildrenTransforms(UIRuntimeObject *parent) {
    // LAYOUT Transform Calculate
    if (parent->hasLayout()) {
        const UIObjectLayoutReference ref(parent);

        auto layoutType = parent->getLayout().type;
        auto& layoutMetadata = UITypes.getTypeMetadata(layoutType);

        layoutMetadata.onComputeChildrenLayout(parent, ref);
    }

    for (auto& rChild : parent->getChildren()) {
        updateUIObject(parent, rChild);
    }

    for (auto& childEntity : parent->getChildren()) {
        updateNodeChildrenTransforms(childEntity);
    }
}

void ui::UIWindowLayoutContext::updateLayout() {
    for (auto dirty : dirtyObjects) {
        updateUIObject(dirty->getParent(), dirty);
        updateNodeChildrenTransforms(dirty);
    }
    dirtyObjects.clear();
}

void ui::UIWindowLayoutContext::addDirty(UIRuntimeObject *obj) {
    UIRuntimeObject* parent = obj->getParent();

    while (parent) {
        if (dirtyObjects.contains(parent)) {
            return;
        }
        parent = parent->getParent();
    }
    dirtyObjects.insert(obj);
}

ui::UIRuntimeObject * ui::UIWindowLayoutContext::getObject(UIRuntimeObject *parent, glm::vec2 position) const {
    for (auto& child : parent->getChildren()) {
        if (child->bounds.contains(position)) {
            if (auto result = getObject(child, position); result) {
                return result;
            }
        }
    }
    return parent;
}

ui::UIRuntimeObject * ui::UIWindowLayoutContext::getObject(glm::vec2 position) const {
    for (auto& root : getRoot()->getChildren()) {
        if (root->bounds.contains(position)) {
            if (const auto result = getObject(root, position)) {
                return result;
            }
        }
    }
    return nullptr;
}

void ui::UIWindowLayoutContext::handleDragMotionEvent(UICursor& cursor, const MouseState &mouse, const MouseMotionEvent &event) {
    if (cursor.isHeld && !cursor.isDragged) {
        if (!cursor.getObject().isDraggable()) return;

        const glm::vec2 mouseDelta = event.position - cursor.clickedPosition;

        const auto result = glm::greaterThanEqual(glm::abs(mouseDelta), cursor.getObject().getDragDeltaRequired());

        if (!glm::all(result)) return;
        cursor.isDragged = true;

        auto& objType = UITypes.getTypeMetadata(cursor.getObject().getType());
        if (!objType.onDrag) return;

        UIDragEvent dEvent(
            UICursorObject(cursor),
            UIDragEventType::BEGIN_DRAGGING,
            UIObject(cursor.getObject().getInternalObject()),
            UIObject(),
            mouse,
            event.position,
            mouseDelta
        );
        objType.onDrag(cursor.getObject().getInternalObject()->object, dEvent);

    } else if (cursor.isDragged) {
        auto& objType = UITypes.getTypeMetadata(cursor.getObject().getType());
        if (!objType.onDrag) return;

        auto targetNode = getObject(event.position);

        if (!targetNode->isDropTargetable()) {
            targetNode = nullptr;
        }

        UIDragEventType evType;

        if (targetNode) {
            if (cursor.getDropTarget() && cursor.getDropTarget() != targetNode) {
                UIDragEvent dEvent(
                    UICursorObject(cursor),
                    UIDragEventType::LEAVE_TARGET,
                    UIObject(cursor.getObject().getInternalObject()),
                    UIObject(cursor.getDropTarget().getInternalObject()),
                    mouse, event.position, event.delta
                );
                objType.onDrag(cursor.getObject().getInternalObject()->object, dEvent);
                cursor.getDropTarget() = targetNode;
                evType = UIDragEventType::ENTER_TARGET;
            } else if (!cursor.getDropTarget()) {
                cursor.getDropTarget() = targetNode;
                evType = UIDragEventType::ENTER_TARGET;
            } else {
                targetNode = nullptr;
                evType = UIDragEventType::OVER_TARGET;
            }
        } else if (cursor.getDropTarget()) {
            evType = UIDragEventType::LEAVE_TARGET;
            targetNode = cursor.getDropTarget().getInternalObject();

            cursor.getDropTarget() = nullptr;
        } else {
            targetNode = nullptr;
            evType = UIDragEventType::DRAGGING;
        }

        UIDragEvent dEvent(
            UICursorObject(cursor),
            evType,
            UIObject(cursor.getObject().getInternalObject()),
            UIObject(targetNode),
            mouse, event.position, event.delta
        );

        objType.onDrag(cursor.getObject().getInternalObject()->object, dEvent);
    }
}

void ui::UIWindowLayoutContext::handleDragEndEvent(UICursor &cursor, const MouseState &mouse, const MouseButtonEvent &event) {
    auto evType = UIDragEventType::END_DRAGGING;

    UIRuntimeObject* targetNode = getObject(event.position);

    if (cursor.getDropTarget()) {
        if (cursor.getDropTarget().getInternalObject()->bounds.contains(event.position)) {
            evType = UIDragEventType::DROPPED;
            targetNode = cursor.getDropTarget().getInternalObject();
        } else {
            if (targetNode->isDropTargetable()) {
                evType = UIDragEventType::DROPPED;
            }
        }
    }

    UIDragEvent dEvent(
        UICursorObject(cursor),
        evType,
        UIObject(cursor.getObject().getInternalObject()),
        UIObject(targetNode),
        mouse,
        event.position,
        mouse.delta
    );

    auto& type = UITypes.getTypeMetadata(cursor.getObject().getType());

    type.onDrag(cursor.getObject().getInternalObject()->object, dEvent);
}


void ui::UIWindowLayoutContext::handleClickEvent(UICursor &cursor, const MouseState& mouse, const MouseButtonEvent &event) {
    auto object = getObject(event.position);

    if (object != UI_NULLPTR) {
        if (event.button == MouseButton::LEFT) {
            if (event.type == MouseInteractionType::PRESS) {
                cursor.getObject() = object;
                cursor.isHeld = true;
                cursor.clickedPosition = event.position;
                cursor.clickedRelativePosition = (event.position - object->bounds.min) / object->bounds.size();
                cursor.clickedOffset = event.position - object->bounds.min;
            }
            else if (event.type == MouseInteractionType::RELEASE) {
                if (cursor.isDragged) {
                    handleDragEndEvent(cursor, mouse, event);
                }
                cursor = {};
            }
        }

        auto& objType = UITypes.getTypeMetadata(object->getType());
        if (!objType.onClick) return;

        UIClickEvent cEvent(
            UICursorObject(cursor),
            UIObject(object),
            mouse,
            event.position,
            event.button,
            event.type,
            event.clicks
        );
        objType.onClick(object->object, cEvent);
    } else {
        if (cursor.isDragged) {
            handleDragEndEvent(cursor, mouse, event);
        }
        cursor = {};
    }
}

void ui::UIWindowLayoutContext::handleMotionEvent(UICursor &cursor, const MouseState& mouse, const MouseMotionEvent &event) {
    if (event.delta == glm::vec2(0)) return;

    handleDragMotionEvent(cursor, mouse, event);

    auto object = getObject(event.position);

    if (!object) return;

    auto& objectType = UITypes.getTypeMetadata(object->getType());

    if (!objectType.onHover) return;

    UIHoverEvent hEvent(
        UICursorObject(cursor),
        UIObject(object),
        mouse,
        event.position
    );
    objectType.onHover(object->object, hEvent);
}

void ui::UIWindowLayoutContext::handleWheelEvent(UICursor &cursor, const MouseState& mouse, const MouseMotionEvent &event) const {
    auto object = getObject(event.position);

    if (!object) return;

    auto& objectType = UITypes.getTypeMetadata(object->getType());

    if (!objectType.onWheel) return;

    UIWheelEvent wEvent(
        UICursorObject(cursor),
        UIObject(object),
        mouse,
        event.delta
    );

    objectType.onWheel(object->object, wEvent);
}