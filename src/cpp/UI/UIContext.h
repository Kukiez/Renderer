#pragma once
#include <vector>
#include <UI/UI.h>

#include "UICursor.h"

struct MouseMotionEvent;
struct MouseButtonEvent;
struct MouseState;

namespace ui {
    struct UIWindowDesc;
    class UIWindowContext;

    class UIAPI UIContext {
        std::vector<UICursor*> cursors;
        std::vector<UIWindowObject*> windows;
    public:
        UIContext() = default;

        UIWindowObject* createWindow(const UIWindowDesc& desc);

        UIWindowObject* getWindow(std::string_view name) const;

        UICursorObject createCursor();
    };
}
