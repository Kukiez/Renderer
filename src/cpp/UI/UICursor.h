#pragma once
#include "UIReference.h"

namespace ui {
    struct UICursor {
        UIWindowObject* window{};
        UIObject object{};
        glm::vec2 clickedOffset{};
        glm::vec2 clickedPosition{};
        glm::vec2 clickedRelativePosition{};
        bool isHeld = false;
        bool isDragged = false;
        UIObject dropTarget{};

        UICursor() = default;

        void clear() {
            window = {};
            object = {};
            clickedOffset = {};
            clickedPosition = {};
            clickedRelativePosition = {};
            isHeld = false;
            isDragged = false;
            dropTarget = {};
        }

        UIObject getObject() const {
            return object;
        }

        UIObject getDropTarget() const {
            return dropTarget;
        }
    };

    class UICursorObject {
        UICursor* cursor{};
    public:
        UICursorObject() = default;
        UICursorObject(UICursor* cursor) : cursor(cursor) {}
        UICursorObject(UICursor& cursor) : cursor(&cursor) {}

        UIObject getActiveObject() const {
            return cursor->object;
        }

        UIObject getDropTarget() const {
            return cursor->dropTarget;
        }

        UICursor* getInternalCursor() const { return cursor; }
    };
}
