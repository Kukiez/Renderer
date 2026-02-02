#pragma once
#include <Renderer/Graphics/GraphicsPass.h>
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Graphics/DrawContainers/ScissorDrawContainer.h>
#include <Renderer/Resource/Geometry/GeometryBuilder.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Texture/TextureQuery.h>
#include <UI/Rendering/UIRenderingContext.h>

#include "UIPassInvocation.h"
#include "UIRenderingPass.h"
#include "UI/Rendering/UIPass.h"
#include "UI/Rendering/UIPassInvocation.h"
#include "UI/Rendering/UIRenderingPass.h"
#include "Renderer/Resource/Shader/ShaderFactory.h"

struct TexturedBox {
    glm::vec4 color = glm::vec4(1);
    glm::vec2 uvMin = glm::vec2(0);
    glm::vec2 uvMax = glm::vec2(1);
    TextureKey texture;
};

struct GPUWidgetInstance {
    glm::vec2 position{};
    glm::vec2 size{};
    glm::vec2 uvMin{};
    glm::vec2 uvMax{};
    glm::vec4 color{};
    unsigned texture{};
    int mode{};
    float borderThickness{};
    int g{};
};

class WidgetRenderer : public SystemComponent {
    GeometryKey rectangleGeometry{};
    ShaderKey shader;
public:
    void onLoad(RendererLoadViewExt level) {
        ShaderFactory shaderFactory(level.getRenderer());
        GeometryFactory geoF(level.getRenderer());

        std::pair<glm::vec2, glm::vec2> v0{ {0.0f, 0.0f}, {0.0f, 0.0f} }; // Bottom-left
        std::pair<glm::vec2, glm::vec2> v1{ {1.0f, 0.0f}, {1.0f, 0.0f} }; // Bottom-right
        std::pair<glm::vec2, glm::vec2> v2{ {1.0f, 1.0f}, {1.0f, 1.0f} }; // Top-right
        std::pair<glm::vec2, glm::vec2> v3{ {0.0f, 1.0f}, {0.0f, 1.0f} }; // Top-left

        std::array vertices{v0, v1, v2, v3};
        std::array<unsigned short, 6> indices{0, 1, 2, 0, 2, 3};

        GeometryDescriptor rectangleDesc(vertices, indices, VertexLayoutOf(VertexAttrib::Position2, VertexAttrib::TexCoord2));
        rectangleGeometry = geoF.loadGeometry(rectangleDesc);

        ui::UIStyleTypes.setRenderer<TexturedBox, WidgetRenderer>();

        shader = shaderFactory.loadShader({
            .vertex = "shaders/UI/ui_rect_geometry.glsl",
            .fragment = "shaders/UI/ui_rect_frag.glsl"
        });
    }

    GeometryKey getGeometry() const { return rectangleGeometry; }

    ShaderKey getWidgetShader() const { return shader; }
    GeometryKey getRectangleGeometry() const { return rectangleGeometry; }
};

struct UIWidgetPipeline : UIPrimitivePipeline<TexturedBox> {
    void onRender(const UIPassInvocation& invocation) const {
        auto& widgetResource = invocation.getRenderer().getSystem<WidgetRenderer>();
        auto& renderer = invocation.getRenderer();

        const GeometryQuery gq(renderer);
        const TextureQuery tq(renderer);

        GraphicsPass& pass = invocation.createRenderPass().createGraphicsPass("ui", widgetResource.getWidgetShader());

        pass.bind(invocation.getCameraBuffer(), "Camera");
        pass.bind(tq.getMaterial2DBuffer(), "MaterialData");

        auto& drawContainer = pass.usingDrawContainer<ScissorDrawContainer>();

        auto visible = invocation.getDrawCommands();

        auto [instancesBuffer, instancesData] = invocation.getRenderer().getBufferStorage().createBufferWithData<GPUWidgetInstance>(invocation.getNumDrawCommands(), BufferUsageHint::FRAME_SCRATCH_BUFFER);

        pass.bind(instancesBuffer, "UIInstances");

        unsigned i = 0;
        for (auto& cmd : mem::make_range(visible, invocation.getNumDrawCommands())) {
            auto& instance = instancesData[i];

            const auto tBox = cmd.is<TexturedBox>();
            instance.position = cmd.getTransform().position;
            instance.size = cmd.getTransform().size;
            instance.uvMax = tBox->uvMin;
            instance.uvMax = tBox->uvMax;
            instance.color = tBox->color;
            instance.mode = 0;
            instance.borderThickness = 0;
            instance.texture = tBox->texture.id();
            ++i;
        }

        ScissorDrawCommand draw;
        draw.geometry = widgetResource.getRectangleGeometry();
        draw.drawRange = gq.getFullDrawRange(draw.geometry);
        draw.drawRange.firstInstance = 0;
        draw.drawRange.instanceCount = invocation.getNumDrawCommands();
        drawContainer.draw(draw);
    }
};