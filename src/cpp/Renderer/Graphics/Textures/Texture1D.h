#pragma once
#include "gl/glew.h"
#include "Texture.h"

class Texture1D {
    unsigned texture = 0;
public:
    Texture1D() = default;

    Texture1D(const Texture1D&) = delete;
    Texture1D& operator = (const Texture1D&) = delete;

    Texture1D(Texture1D&& other) noexcept : texture(other.texture) {
        other.texture = 0;
    }

    Texture1D& operator = (Texture1D&& other) noexcept {
        if (this != &other) {
            discard();
            texture = other.texture;
            other.texture = 0;
        }
        return *this;
    }

    ~Texture1D() {
        discard();
    }

    void discard() {
        if (texture) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
    }

    unsigned id() const {
        return texture;
    }
};