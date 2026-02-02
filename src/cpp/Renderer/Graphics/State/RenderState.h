#pragma once

enum class ClearTarget {
    NONE = 0,
    COLOR = 1,
    STENCIL = 2,
    DEPTH = 4,
    ALL = COLOR | STENCIL | DEPTH
};

enum class BlendFunction : uint8_t {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate
};

enum class BlendEquation : uint8_t {
    FuncAdd,
    FuncSubtract,
    FuncReverseSubtract,
    Min,
    Max
};

enum class StencilFunction : uint8_t {
    Never,
    Less,
    Lequal,
    Greater,
    Gequal,
    Equal,
    Notequal,
    Always
};

enum class StencilOp : uint8_t {
    Keep,
    Zero,
    Replace,
    Incr,
    IncrWrap,
    Decr,
    DecrWrap,
    Invert
};

enum class DepthFunction : uint8_t {
    Never,
    Less,
    Lequal,
    Greater,
    Gequal,
    Equal,
    Notequal,
    Always
};


enum class CullMode : uint8_t {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class FrontFace : uint8_t {
    Ccw,
    Cw
};

enum class PolygonMode : uint8_t {
    Points,
    Wireframe,
    Solid
};

struct BlendState {
    BlendFunction srcRGB = BlendFunction::SrcAlpha;
    BlendFunction dstRGB = BlendFunction::OneMinusSrcAlpha;
    BlendFunction srcAlpha = BlendFunction::One;
    BlendFunction dstAlpha = BlendFunction::OneMinusSrcAlpha;
    BlendEquation rgbEquation = BlendEquation::FuncAdd;
    BlendEquation alphaEquation = BlendEquation::FuncAdd;

    bool enabled = false;

    bool operator == (const BlendState& other) const {
        return memcmp(this, &other, sizeof(BlendState)) == 0;
    }

    bool operator != (const BlendState& other) const {
        return !(*this == other);
    }
};

struct StencilState {
    StencilFunction func = StencilFunction::Always;
    unsigned ref = 0;
    unsigned mask = 0xFF;

    StencilOp fail = StencilOp::Keep;
    StencilOp zfail = StencilOp::Keep;
    StencilOp zpass = StencilOp::Keep;

    bool enabled = false;

    bool operator==(const StencilState& other) const {
        return enabled == other.enabled &&
               func   == other.func &&
               ref    == other.ref &&
               mask   == other.mask &&
               fail   == other.fail &&
               zfail  == other.zfail &&
               zpass  == other.zpass;
    }
};

struct DepthState {
    DepthFunction func = DepthFunction::Less;
    bool writeEnabled = true;
    bool enabled = false;

    bool operator==(const DepthState& other) const {
        return enabled     == other.enabled &&
                writeEnabled == other.writeEnabled &&
               func        == other.func;
    }
};

struct ScissorRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr bool operator==(const ScissorRect& other) const {
        return x == other.x && y == other.y &&
               width == other.width && height == other.height;
    }
};

struct ScissorState {
    ScissorRect rect = {};
    bool enabled = false;

    bool operator==(const ScissorState & other) const {
        return enabled == other.enabled && rect == other.rect;
    }
};

struct Viewport {
    bool enabled = false;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const Viewport& other) const {
        return enabled == other.enabled &&
               x == other.x && y == other.y &&
               width == other.width && height == other.height;
    }
};

template <> struct std::hash<ScissorState> {
    size_t operator()(const ScissorState& r) const noexcept {
        size_t h = 0;
        cexpr::hash_combine(h, r.enabled);
        cexpr::hash_combine(h, r.rect.x);
        cexpr::hash_combine(h, r.rect.y);
        cexpr::hash_combine(h, r.rect.width);
        cexpr::hash_combine(h, r.rect.height);
        return h;
    }
};

template <> struct std::hash<Viewport> {
    size_t operator () (const Viewport& vp) const noexcept {
        size_t h = 0;
        cexpr::hash_combine(h, vp.enabled);
        cexpr::hash_combine(h, vp.x);
        cexpr::hash_combine(h, vp.y);
        cexpr::hash_combine(h, vp.width);
        cexpr::hash_combine(h, vp.height);
        return h;
    }
};

struct RenderState {
    enum State {
        BLEND_STATE,
        DEPTH_STATE,
        CULL_MODE,
        STENCIL_STATE,
        FRONT_FACE,
        POLYGON_MODE,
        SCISSOR_STATE,
        VIEWPORT_STATE,
        NUM_STATES
    };

    BlendState blendState{};
    DepthState depthState{};
    CullMode cullMode{};
    StencilState stencilState{};
    FrontFace frontFace = FrontFace::Ccw;
    PolygonMode polygonMode = PolygonMode::Solid;
    ScissorState scissorState{};
    Viewport viewport{};

    bool operator == (const RenderState& other) const {
        return blendState == other.blendState &&
            depthState == other.depthState &&
            cullMode   == other.cullMode &&
            stencilState == other.stencilState &&
            frontFace  == other.frontFace &&
            polygonMode == other.polygonMode &&
            scissorState == other.scissorState &&
            viewport == other.viewport;
    }

    template <typename FunctionTable>
    bool difference(const RenderState& newState, FunctionTable&& table) const {
        bool anyDiff = false;

        if (blendState != newState.blendState) {
            table(blendState, newState.blendState);
            anyDiff = true;
        }
        if (depthState != newState.depthState) {
            table(depthState, newState.depthState);
            anyDiff = true;
        }
        if (cullMode != newState.cullMode) {
            table(cullMode, newState.cullMode);
            anyDiff = true;
        }
        if (stencilState != newState.stencilState) {
            table(stencilState, newState.stencilState);
            anyDiff = true;
        }
        if (frontFace != newState.frontFace) {
            table(frontFace, newState.frontFace);
            anyDiff = true;
        }
        if (polygonMode != newState.polygonMode) {
            table(polygonMode, newState.polygonMode);
            anyDiff = true;
        }
        if (scissorState != newState.scissorState) {
            table(scissorState, newState.scissorState);
            anyDiff = true;
        }
        if (viewport != newState.viewport) {
            table(viewport, newState.viewport);
            anyDiff = true;
        }
        return anyDiff;
    }

    size_t hash() const {
        size_t h = 0;

        // BlendState
        cexpr::hash_combine(h, static_cast<size_t>(blendState.srcRGB));
        cexpr::hash_combine(h, static_cast<size_t>(blendState.dstRGB));
        cexpr::hash_combine(h, static_cast<size_t>(blendState.srcAlpha));
        cexpr::hash_combine(h, static_cast<size_t>(blendState.dstAlpha));
        cexpr::hash_combine(h, static_cast<size_t>(blendState.rgbEquation));
        cexpr::hash_combine(h, static_cast<size_t>(blendState.alphaEquation));
        cexpr::hash_combine(h, static_cast<size_t>(blendState.enabled));

        // DepthState
        cexpr::hash_combine(h, static_cast<size_t>(depthState.writeEnabled));
        cexpr::hash_combine(h, static_cast<size_t>(depthState.func));
        cexpr::hash_combine(h, static_cast<size_t>(depthState.enabled));

        // Cull & FrontFace
        cexpr::hash_combine(h, static_cast<size_t>(cullMode));
        cexpr::hash_combine(h, static_cast<size_t>(frontFace));

        // StencilState
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.func));
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.ref));
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.mask));
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.fail));
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.zfail));
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.zpass));
        cexpr::hash_combine(h, static_cast<size_t>(stencilState.enabled));

        // ScissorState
        cexpr::hash_combine(h, std::hash<ScissorState>{}(scissorState));
        cexpr::hash_combine(h, static_cast<size_t>(scissorState.enabled));

        // Polygon & Viewport
        cexpr::hash_combine(h, static_cast<size_t>(polygonMode));
        cexpr::hash_combine(h, std::hash<Viewport>{}(viewport));

        return h;
    }
};

template <> struct std::hash<RenderState> {
    size_t operator()(const RenderState& state) const noexcept {
        return state.hash();
    }
};