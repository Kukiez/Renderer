#include "UIContext.h"
#include "Window/UIWindowContext.h"

ui::UIWindowObject * ui::UIContext::createWindow(const UIWindowDesc& desc) {
    auto back = windows.emplace_back(new UIWindowObject(desc));

    return back;
}

ui::UIWindowObject * ui::UIContext::getWindow(std::string_view name) const {
    for (auto& window : windows) {
        if (window->getName() == name) {
            return window;
        }
    }
    return nullptr;
}

ui::UICursorObject ui::UIContext::createCursor() {
    return cursors.emplace_back(new UICursor);
}
