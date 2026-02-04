#pragma once
#include "RendererAPI.h"

struct SDL_Window;
struct SDL_Renderer;

class ApplicationWindow {
    RENDERERAPI bool initSDL(int sizeX, int sizeY);

    RENDERERAPI void destroySDL() const;

    SDL_Window* window;
    SDL_Renderer* renderer;
    void* glContext;

    static inline int frameIndex = 0;
public:
    ApplicationWindow() = default;

    void init(int width, int height) {
        initSDL(width, height);
    }

    RENDERERAPI void swapWindow() const;

    void destroy() const {
        destroySDL();
    }

    auto getWindow() const {
        return window;
    }

    static int getFrameIndex() {
        return frameIndex - 1;
    }
};