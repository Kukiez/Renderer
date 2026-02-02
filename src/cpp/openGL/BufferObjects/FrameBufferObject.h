#pragma once
#include <gl/glew.h>
#include <Image/CubemapImage.h>
#include <Image/Image.h>
#include <memory/Span.h>
#include <Renderer/Graphics/Textures/Texture.h>

enum class FramebufferAttachmentIndex : uint8_t {
    COLOR_0,
    COLOR_1,
    COLOR_2,
    COLOR_3,
    COLOR_4,
    COLOR_5,
    COLOR_6,
    COLOR_7,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL,
    MAX_ATTACHMENTS
};

constexpr GLenum opengl_enum_cast(const FramebufferAttachmentIndex type) {
    switch (type) {
        case FramebufferAttachmentIndex::DEPTH:
            return GL_DEPTH_ATTACHMENT;
        case FramebufferAttachmentIndex::STENCIL:
            return GL_STENCIL_ATTACHMENT;
        case FramebufferAttachmentIndex::DEPTH_STENCIL:
            return GL_DEPTH_STENCIL_ATTACHMENT;
        default:
            return GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(type);
    }
}

struct FramebufferAttachmentCreateParams {
    TextureTarget type = TextureTarget::TEXTURE_2D;
    unsigned texture = 0;
    int level = 0;

    FramebufferAttachmentIndex attachmentIndex{};

    CubemapFace face{}; // if TextureTarget is CUBEMAP
    unsigned arrayIndex = 0; // if TextureTarget is an array
};

constexpr GLenum opengl_enum_cast(CubemapFace face) {
    switch (face) {
        case CubemapFace::POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case CubemapFace::NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case CubemapFace::POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case CubemapFace::NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case CubemapFace::POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case CubemapFace::NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    }
}

class FrameBufferObject {
    void genFramebuffer() {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    }

    unsigned FBO = 0;
    std::array<GLenum, 8> drawBufferAttachments{};
    uint8_t drawBufferCount = 0;
    int width = 0, height = 0;
public:
    FrameBufferObject() = default;

    static FrameBufferObject empty(const int width, const int height) {
        FrameBufferObject FBO;
        FBO.width = width;
        FBO.height = height;
        glGenFramebuffers(1, &FBO.FBO);
        return FBO;
    }

    FrameBufferObject(const int width, const int height, const FramebufferAttachmentCreateParams& param) : width(width), height(height) {
        genFramebuffer();
        setAttachment(param);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer incomplete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    explicit FrameBufferObject(const int width, const int height, mem::range<FramebufferAttachmentCreateParams> params) : width(width), height(height) {
        genFramebuffer();

        for (int i = 0; i < params.size(); ++i) {
            setAttachment(params[i]);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer incomplete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    FrameBufferObject(const FrameBufferObject &other) = delete;
    FrameBufferObject &operator=(const FrameBufferObject &other) = delete;

    FrameBufferObject(FrameBufferObject &&other) noexcept
    : FBO(other.FBO), drawBufferAttachments(other.drawBufferAttachments), drawBufferCount(other.drawBufferCount), width(other.width), height(other.height) {
        other.FBO = 0;
        other.drawBufferCount = 0;
        other.width = 0;
        other.height = 0;
    }

    FrameBufferObject &operator=(FrameBufferObject &&other) noexcept {
        if (this != &other) {
            FBO = other.FBO;
            drawBufferAttachments = other.drawBufferAttachments;
            drawBufferCount = other.drawBufferCount;
            width = other.width;
            height = other.height;

            other.FBO = 0;
            other.drawBufferCount = 0;
            other.width = 0;
            other.height = 0;
        }
        return *this;
    }

    ~FrameBufferObject() {
        if (FBO) glDeleteFramebuffers(1, &FBO);
    }

    void setAttachment(const FramebufferAttachmentCreateParams& param) {
        if (param.texture) {
            GLenum attachment;
            if (param.attachmentIndex <= FramebufferAttachmentIndex::COLOR_7) {
                attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(param.attachmentIndex);
                drawBufferAttachments[drawBufferCount++] = attachment;
            } else {
                attachment = opengl_enum_cast(param.attachmentIndex);
            }
            switch (param.type) {
                case TextureTarget::TEXTURE_2D:
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, param.texture, param.level);
                    break;
                case TextureTarget::TEXTURE_2D_MSAA:
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, param.texture, 0);
                    break;
                case TextureTarget::TEXTURE_ARRAY_2D:
                    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, param.texture, param.level, param.arrayIndex);
                    break;
                case TextureTarget::TEXTURE_3D:
                    glFramebufferTexture3D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_3D, param.texture, param.level, param.arrayIndex);
                    break;
                case TextureTarget::TEXTURE_1D:
                    glFramebufferTexture1D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_1D, param.texture, param.level);
                    break;
                case TextureTarget::CUBEMAP:
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, opengl_enum_cast(param.face), param.texture, param.level);
                    break;
                default: {
                    assert(false);
                    std::cerr << "Unsupported Texture Type" << std::endl;
                    break;
                };
            }
        }
    }

    void discard() {
        if (FBO != 0) {
            glDeleteFramebuffers(1, &FBO);
            FBO = 0;
        }
    }

    void bind() const {
        if (FBO) {
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glViewport(0, 0, width, height);

            if (drawBufferCount > 0) {
                glDrawBuffers(drawBufferCount, drawBufferAttachments.data());
            }
            else {
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            }
        }
    }

    unsigned id() const {
        return FBO;
    }
};