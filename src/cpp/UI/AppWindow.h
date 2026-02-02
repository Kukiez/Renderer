#pragma once
#include <bitset>
#include <SDL2/SDL_scancode.h>
#include <glm/vec2.hpp>


enum class MouseButton {
    NONE = 0,
    LEFT = 1 << 0,
    MIDDLE = 1 << 1,
    RIGHT = 1 << 2,
    X1 = 1 << 3,
    X2 = 1 << 4
};

enum class MouseModifiers {
    LSHIFT = 1,
    RSHIFT = 2,
    LCTRL = 4,
    RCTRL = 8,
    LALT = 16,
    RALT = 32
};

enum class MouseInteractionType {
    PRESS,
    RELEASE
};

struct MouseState {
    glm::vec2 position{};
    glm::vec2 delta{};
    MouseButton buttons{};

    bool isButtonDown(MouseButton button) const {
        return (static_cast<int>(buttons) & static_cast<int>(button)) != 0;
    }

    bool isButtonUp(MouseButton button) const {
        return !isButtonDown(button);
    }
};

struct MouseButtonEvent {
    MouseInteractionType type{};
    MouseButton button{};
    MouseInteractionType state{};
    uint8_t clicks{};
    glm::vec2 position{};

    MouseModifiers modifiers{};
};

struct MouseMotionEvent {
    glm::vec2 position{};
    glm::vec2 delta{};
};

struct KeyboardState {
    std::bitset<SDL_Scancode::SDL_NUM_SCANCODES> keys{};

    bool isKeyDown(SDL_Scancode key) const {
        return keys.test(key);
    }

    bool isKeyUp(SDL_Scancode key) const {
        return !isKeyDown(key);
    }
};

struct ApplicationWindowInput {
    MouseState mouse{};
    MouseMotionEvent mouseMotion{};
    MouseMotionEvent wheelMotion{};

    KeyboardState keyboard{};
    std::vector<MouseButtonEvent> mouseButtons{};

    void clear() {
        mouseMotion = {};
        wheelMotion = {};
        mouseButtons.clear();
    }
};