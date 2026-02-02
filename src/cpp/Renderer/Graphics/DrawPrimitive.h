#pragma once

enum class DrawPrimitive : uint8_t {
    POINTS,
    LINES,
    LINES_STRIP,
    LINES_LOOP,
    TRIANGLES,
    TRIANGLES_STRIP,
    TRIANGLES_FAN,
    PATCHES
};