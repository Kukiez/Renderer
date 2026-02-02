#pragma once
#include "UI/AppWindow.h"
#include "UI/UIReference.h"

namespace ui {
    struct UIClickEvent {
        UICursorObject cursor;
        UIObject object;
        MouseState mouse;
        glm::vec2 position{};
        MouseButton button{};
        MouseInteractionType type{};
        uint8_t clicks{};
        bool propagate = false;
    };

    struct UIHoverEvent {
        UICursorObject cursor;
        UIObject object;
        MouseState mouse;
        glm::vec2 position{};
    };

    struct UIWheelEvent {
        UICursorObject cursor;
        UIObject object;
        MouseState mouse;
        glm::vec2 position{};
        glm::vec2 delta{};
    };

    enum class UIDragEventType {
        NONE = 0,

        BEGIN_DRAGGING = 1 << 0,
        DRAGGING = 1 << 1,
        END_DRAGGING = 1 << 2,

        ENTER_TARGET = 1 << 3,
        LEAVE_TARGET = 1 << 4,
        OVER_TARGET = 1 << 5,

        DROPPED = 1 << 6
    };

    struct UIDragEvent {
        UICursorObject cursor;
        UIDragEventType type{};
        UIObject source;
        UIObject target;
        MouseState mouse{};
        glm::vec2 position{};
        glm::vec2 delta{};
    };
}

