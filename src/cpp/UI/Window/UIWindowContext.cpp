#include "UIWindowContext.h"

void ui::UIWindowContext::addObject(UIObject obj) {
    if (obj.getParent()) return;

    addRootObject(obj);
}

void ui::UIWindowContext::destroyObject(const UIObject obj) {
    auto objPtr = obj.getInternalObject();

    if constexpr (UI_DEBUG) {
        for (auto free : freeObjects) {
            if (free == objPtr) {
                assert(false && "UIObject double-free");
            }
        }
    }

    auto type = objPtr->getType();
    void* objData = objPtr->object;

    allocCtx.deallocate(objData, UITypes.getTypeMetadata(type).type, 1);

    if (objPtr->layout.layoutObj) {
        auto layoutType = objPtr->getLayout().type;
        auto& layoutMetadata = UITypes.getTypeMetadata(layoutType);
        allocCtx.deallocate(objPtr->layout.layoutObj, layoutMetadata.type, 1);
    }

    detachObject(obj);
    freeObjects.push_back(objPtr);
}

void ui::UIWindowContext::render(UIRenderingContext &ctx) const {
    for (auto& root : layout.getRoot()->getChildren()) {
        auto& typeMeta = UITypes.getTypeMetadata(root->getType());

        if (typeMeta.onRender) {
            typeMeta.onRender(root->object, UIObject(root), ctx);
        }
    }
}

bool ui::UIWindowContext::clickCursor(UICursorObject cursor, const MouseState& mouse, const MouseButtonEvent &button) {
    cursor.getInternalCursor()->window = layout.getRoot();
    layout.handleClickEvent(*cursor.getInternalCursor(), mouse, button);
    return true;
}

bool ui::UIWindowContext::moveCursor(UICursorObject cursor, const MouseState &mouse, const MouseMotionEvent &motion) {
    cursor.getInternalCursor()->window = layout.getRoot();
    layout.handleMotionEvent(*cursor.getInternalCursor(), mouse, motion);
    return true;
}

bool ui::UIWindowContext::moveCursorWheel(UICursorObject cursor, const MouseState &mouse, const MouseMotionEvent &wheel) {
    cursor.getInternalCursor()->window = layout.getRoot();
    layout.handleWheelEvent(*cursor.getInternalCursor(), mouse, wheel);
    return true;
}





//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






void ui::UIWindowContext::detachObjectImpl(UIRuntimeObject**& children, size_t& numChildren, const UIRuntimeObject *child, size_t hint) {
    if (hint == numChildren - 1) {
        numChildren -= 1;
        return;
    }
    if (hint < numChildren) {
        children[hint] = children[numChildren - 1];
        numChildren -= 1;
        return;
    }

    for (size_t i = 0; i < numChildren; ++i) {
        if (children[i] == child) {
            if (numChildren - 1 != i) {
                children[i] = children[numChildren - 1];
            }
            numChildren -= 1;
        }
    }
}

void ui::UIWindowContext::detachObject(const UIObject child, size_t hint) {

    if (child.getParent()) {
        auto parent = child.getParent();

        detachObjectImpl(parent.getInternalObject()->children, parent.getInternalObject()->numChildren, child.getInternalObject(), hint);
    } else {
        detachObjectImpl(layout.getRoot()->children, layout.getRoot()->numChildren, child.getInternalObject(), hint);
    }
}

size_t ui::UIWindowContext::findChildIndex(UIRuntimeObject ** children, size_t numChildren,
    const UIRuntimeObject *child)
{
    for (size_t i = 0; i < numChildren; ++i) {
        if (children[i] == child) return i;
    }
    return numChildren;
}

void ui::UIWindowContext::setObjectParentImpl(UIRuntimeObject **& parentChildren, size_t &numChildren,  UIRuntimeObject *obj)
{
    const size_t childrenCount = numChildren;

    auto** newChildren = new UIRuntimeObject*[childrenCount + 1];

    for (size_t i = 0; i < childrenCount; ++i) {
        newChildren[i] = parentChildren[i];
    }

    newChildren[childrenCount] = obj;

    parentChildren = newChildren;
    numChildren += 1;
}

void ui::UIWindowContext::setObjectParent(const UIObject parent, const UIObject child) {
    auto& parentChildren = parent.getInternalObject()->children;
    auto& numChildren = parent.getInternalObject()->numChildren;

    setObjectParentImpl(parentChildren, numChildren, child.getInternalObject());

    auto pInternal = parent.getInternalObject();
    auto cInternal = child.getInternalObject();

    layout.addDirty(pInternal);

    if (cInternal->parent) {
        layout.addDirty(cInternal->parent);
        detachObject(child);
    } else {
        detachObjectImpl(layout.getRoot()->children, layout.getRoot()->numChildren, child.getInternalObject());
    }

    cInternal->parent = pInternal;
}

void ui::UIWindowContext::addRootObject(const UIObject obj) {

    auto& parentChildren = layout.getRoot()->children;
    auto& numChildren = layout.getRoot()->numChildren;

    assert(obj.getParent() == nullptr);

    setObjectParentImpl(parentChildren, numChildren, obj.getInternalObject());
    layout.addDirty(obj.getInternalObject());
}

ui::UIRuntimeObject * ui::UIWindowContext::findObjectSlot() {
    if (!freeObjects.empty()) {
        auto back = freeObjects.back();
        freeObjects.pop_back();
        return back;
    }

    auto* back = &uiObjects.back();

    if (back->numObjects != back->capObjects) {
        return &back->objects[back->numObjects++];
    }

    const size_t newSize = back->capObjects * 2;
    back = &uiObjects.emplace_back(UIObjectArray(new UIRuntimeObject[newSize], 0, newSize));

    return &back->objects[back->numObjects++];
}