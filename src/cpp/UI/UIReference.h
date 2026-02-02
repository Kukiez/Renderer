#pragma once
#include <UI/UI.h>

namespace ui {
    static constexpr auto UI_NULLPTR = nullptr;

    class UIObjectLayoutReference : public UIObject {
    public:
        UIObjectLayoutReference(UIRuntimeObject* object) : UIObject(object) {}

        void setLayoutTransform(const UITransform transform) const {
            getInternalObject()->layout.transform = transform;
        }
    };
}
