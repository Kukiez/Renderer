#pragma once
#include "RenderState.h"
#include <Util/enum_bit.h>
constexpr GLenum opengl_enum_cast(BlendFunction func) {
    constexpr static GLenum arr[] = {
        GL_ZERO,
        GL_ONE,
        GL_SRC_COLOR,
        GL_ONE_MINUS_SRC_COLOR,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,
        GL_ONE_MINUS_DST_ALPHA,
        GL_CONSTANT_COLOR,
        GL_ONE_MINUS_CONSTANT_COLOR,
        GL_CONSTANT_ALPHA,
        GL_ONE_MINUS_CONSTANT_ALPHA,
        GL_SRC_ALPHA_SATURATE
    };
    return arr[static_cast<size_t>(func)];
}

constexpr GLenum opengl_enum_cast(ClearTarget target) {
    GLenum result = 0;

    if ((target & ClearTarget::COLOR) == ClearTarget::COLOR) {
        result |= GL_COLOR_BUFFER_BIT;
    }

    if ((target & ClearTarget::STENCIL) == ClearTarget::STENCIL) {
        result |= GL_STENCIL_BUFFER_BIT;
    }

    if ((target & ClearTarget::DEPTH) == ClearTarget::DEPTH) {
        result |= GL_DEPTH_BUFFER_BIT;
    }
    return result;
}

constexpr GLenum opengl_enum_cast(PolygonMode mode) {
    constexpr GLenum arr[] = {
        GL_POINT,
        GL_LINE,
        GL_FILL
    };
    return arr[static_cast<size_t>(mode)];
}

constexpr GLenum opengl_enum_cast(BlendEquation eq) {
    constexpr static GLenum arr[] = {
        GL_FUNC_ADD,
        GL_FUNC_SUBTRACT,
        GL_FUNC_REVERSE_SUBTRACT,
        GL_MIN,
        GL_MAX
    };
    return arr[static_cast<size_t>(eq)];
}

constexpr GLenum opengl_enum_cast(StencilFunction func) {
    constexpr GLenum arr[] = {
        GL_NEVER,
        GL_LESS,
        GL_LEQUAL,
        GL_GREATER,
        GL_GEQUAL,
        GL_EQUAL,
        GL_NOTEQUAL,
        GL_ALWAYS
    };
    return arr[static_cast<size_t>(func)];
}

constexpr GLenum opengl_enum_cast(StencilOp op) {
    constexpr GLenum arr[] = {
        GL_KEEP,
        GL_ZERO,
        GL_REPLACE,
        GL_INCR,
        GL_INCR_WRAP,
        GL_DECR,
        GL_DECR_WRAP,
        GL_INVERT
    };
    return arr[static_cast<size_t>(op)];
}

constexpr GLenum opengl_enum_cast(DepthFunction func) {
    constexpr static GLenum arr[] = {
        GL_NEVER,
        GL_LESS,
        GL_LEQUAL,
        GL_GREATER,
        GL_GEQUAL,
        GL_EQUAL,
        GL_NOTEQUAL,
        GL_ALWAYS
    };
    return arr[static_cast<size_t>(func)];
}

constexpr GLenum opengl_enum_cast(FrontFace face) {
    constexpr GLenum arr[] = {
        GL_CCW, // Ccw
        GL_CW   // Cw
    };
    return arr[static_cast<size_t>(face)];
}

constexpr GLenum opengl_enum_cast(CullMode mode) {
    constexpr GLenum arr[] = {
        0,
        GL_FRONT,
        GL_BACK,
        GL_FRONT_AND_BACK
    };
    return arr[static_cast<size_t>(mode)];
}