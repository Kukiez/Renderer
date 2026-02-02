#pragma once
#include "DrawPrimitive.h"

constexpr GLenum opengl_enum_cast(const DrawPrimitive primitive) {
    switch (primitive) {
        case DrawPrimitive::POINTS:
            return GL_POINTS;

        case DrawPrimitive::LINES:
            return GL_LINES;

        case DrawPrimitive::LINES_STRIP:
            return GL_LINE_STRIP;

        case DrawPrimitive::LINES_LOOP:
            return GL_LINE_LOOP;

        case DrawPrimitive::TRIANGLES:
            return GL_TRIANGLES;

        case DrawPrimitive::TRIANGLES_STRIP:
            return GL_TRIANGLE_STRIP;

        case DrawPrimitive::TRIANGLES_FAN:
            return GL_TRIANGLE_FAN;

        case DrawPrimitive::PATCHES:
            return GL_PATCHES;
    }
    assert(false);
    std::unreachable();
}