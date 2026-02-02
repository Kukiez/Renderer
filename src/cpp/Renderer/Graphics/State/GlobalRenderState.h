#pragma once
#include "RenderState.h"
#include "GLRenderState.h"

inline struct OpenGLStateApplication {
    template <typename... Ls>
    struct overloads : Ls... {
        overloads(Ls... ls) {}

        using Ls::operator()...;
    };

    struct BlendBinder {
        void operator()(const BlendState& curr, const BlendState& newState) const {
            if (newState.enabled) {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(opengl_enum_cast(newState.srcRGB),
                                    opengl_enum_cast(newState.dstRGB),
                                    opengl_enum_cast(newState.srcAlpha),
                                    opengl_enum_cast(newState.dstAlpha));
                glBlendEquationSeparate(opengl_enum_cast(newState.rgbEquation),
                                        opengl_enum_cast(newState.alphaEquation));
            } else {
                glDisable(GL_BLEND);
            }
        }
    };

    struct DepthBinder {
        void operator()(const DepthState& curr, const DepthState& newState) const {
            if (newState.enabled) {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(opengl_enum_cast(newState.func));
                glDepthMask(newState.writeEnabled ? GL_TRUE : GL_FALSE);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
        }
    };

    struct CullBinder {
        void operator()(CullMode curr, CullMode newState) const {
            if (newState != CullMode::None) {
                glEnable(GL_CULL_FACE);
                glCullFace(opengl_enum_cast(newState));
            } else {
                glDisable(GL_CULL_FACE);
            }
        }
    };

    struct StencilBinder {
        void operator()(const StencilState& curr, const StencilState& newState) const {
            if (newState.enabled) {
                glEnable(GL_STENCIL_TEST);
                glStencilFunc(opengl_enum_cast(newState.func), newState.ref, newState.mask);
                glStencilOp(opengl_enum_cast(newState.fail),
                            opengl_enum_cast(newState.zfail),
                            opengl_enum_cast(newState.zpass));
            } else {
                glDisable(GL_STENCIL_TEST);
            }
        }
    };

    struct FrontFaceBinder {
        void operator()(FrontFace curr, FrontFace newState) const {
            glFrontFace(opengl_enum_cast(newState));
        }
    };

    struct PolygonModeBinder {
        void operator()(PolygonMode curr, PolygonMode newState) const {
            glPolygonMode(GL_FRONT_AND_BACK, opengl_enum_cast(newState));
        }
    };

    struct ScissorBinder {
        void operator()(const ScissorState& curr, const ScissorState& newState) const {
            if (newState.enabled) {
                glEnable(GL_SCISSOR_TEST);
                const auto& r = newState.rect;
                glScissor(r.x, r.y, r.width, r.height);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
        }
    };

    struct ViewportBinder {
        void operator()(const Viewport& curr, const Viewport& newState) const {
            if (newState.enabled) {
                glViewport(newState.x, newState.y, newState.width, newState.height);
            }
        }
    };

    static inline auto ApplicationTable = overloads{
        BlendBinder{},
        DepthBinder{},
        CullBinder{},
        StencilBinder{},
        FrontFaceBinder{},
        PolygonModeBinder{},
        ScissorBinder{},
        ViewportBinder{}
    };

    void change(const RenderState& newState) {
        if (current.difference(newState, ApplicationTable))
            current = newState;
    }

    RenderState current;
} OPENGL_STATE_APPLICATION;
