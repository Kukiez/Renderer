#pragma once
#include <UI/Text/UIFontLoader.h>
#include <UI/Rendering/UIStyleType.h>
#include <Renderer/Graphics/DrawContainers/ScissorDrawContainer.h>
#include <Renderer/Resource/Texture/TextureFactory.h>
#include <UI/Text/UIText.h>

#include "WidgetRenderer.h"

struct UITextMaterial {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 uvOffset;
    glm::vec2 uvScale;
    glm::vec4 color;
    unsigned texture;
    int mode;
    float borderThickness;
    unsigned font;
    glm::vec4 charUV;
};

struct TextStyle  {
    using ComponentType = ui::UIStyleType;
    const ui::UITextStream* text;
    const ui::UITextLayout* layout;
};

class TextRenderer : public SystemComponent {
    ShaderKey textShader{};

    TextureKey fontTexture{};

    std::vector<glm::vec4> glyphUVs;
    FontAsset fontAsset{};

    BufferKey textInstances;
public:
    void onLoad(RendererLoadViewExt view) {
        ShaderFactory sf(view.getRenderer());
        TextureFactory tf(view.getRenderer());

        auto [font, rFont] = UIFontLoader::cookFont("assets/fonts/JetBrainsMono-Regular.ttf", "JetBrains", 256);

        TextureCreateParams fontParams;
        fontParams.format = TextureFormat::R_8;

        fontTexture = tf.createTexture2D(TextureDescriptor2D(fontParams, std::move(rFont.glyphImage)));

        textShader = sf.loadShader({
            .vertex = "shaders/UI/ui_text_vertex.glsl",
            .fragment = "shaders/UI/ui_text_frag.glsl"
        });

        glyphUVs = std::move(rFont.glyphUVs);

        ui::UIStyleTypes.setRenderer<TextStyle, TextRenderer>();

        fontAsset = std::move(font);

        textInstances = view.getRenderer().getBufferStorage().createBuffer<UITextMaterial>(5415, BufferUsageHint::PERSISTENT_WRITE_ONLY);
    }

    unsigned createTextContent(const TextStyle& style, glm::vec2 textPosition, glm::vec2 textScale, mem::range<UITextMaterial> outTextMaterials) {
        unsigned cmdIdx = 0;
        for (auto& run : style.layout->getRuns()) {
            float scale = static_cast<float>(run.style->font.size) / static_cast<float>(fontAsset.getResolution());

            for (auto texCh : run.getText(style.text->getText())) {
                UITextMaterial& instance = outTextMaterials[cmdIdx++];

                auto glyph = fontAsset.getGlyph(texCh);

                float chPosX = textPosition.x + static_cast<float>(glyph.bearing.x) * scale;
                float chPosY = textPosition.y + static_cast<float>(fontAsset.getResolution() - glyph.bearing.y) * scale;

                instance.position = {chPosX, chPosY};
                instance.size = glm::vec2{glyph.size} * scale;

                textPosition.x += static_cast<float>(glyph.advance) * scale;

                instance.uvOffset = glm::vec2(0);
                instance.uvScale = glm::vec2(1.0f);
                instance.color = glm::vec4(run.style->color, run.style->opacity);
                instance.texture = 0;
                instance.mode = 0;
                instance.borderThickness = 0;
                instance.charUV = glyphUVs[texCh];
                instance.font = fontTexture.id();
            }
        }
        return cmdIdx;
    }

    ShaderKey getTextShader() const { return textShader; }

    BufferKey getTextInstancesBuffer() const { return textInstances; }
};

struct UITextPipeline : UIPrimitivePipeline<TextStyle> {
    void onRender(const UIPassInvocation& invocation) const {
        auto& textRenderer = invocation.getRenderer().getSystem<TextRenderer>();
        auto& renderer = invocation.getRenderer();

        GraphicsPass& pass = invocation.createRenderPass().createGraphicsPass("UITexts", textRenderer.getTextShader());

        auto& widgetRenderer = invocation.getRenderer().getSystem<WidgetRenderer>();

        TextureQuery tq(renderer);
        GeometryQuery gq(renderer);

        pass.bind(invocation.getCameraBuffer(), "Camera");
        pass.bind(tq.getMaterial2DBuffer(), "MaterialData");

        auto visible = invocation.getDrawCommands();
        const size_t numVisibleCmds = invocation.getNumDrawCommands();

        unsigned numInstances = 0;
        for (auto& cmd : mem::make_range(visible, numVisibleCmds)) {
            const auto* style = cmd.is<TextStyle>();
            numInstances += style->text->size();
        }

        auto [textInstancesBuffer, textInstancesData] = renderer.getBufferStorage().createBufferWithData<UITextMaterial>(numInstances, BufferUsageHint::FRAME_SCRATCH_BUFFER);
        pass.bind(textInstancesBuffer, "UIInstances");

        unsigned cmdIdx = 0;
        for (auto& cmd : mem::make_range(visible, numVisibleCmds)) {
            const auto* style = cmd.is<TextStyle>();

            auto textMaterials = textInstancesData.span(cmdIdx);
            textRenderer.createTextContent(*style, cmd.getTransform().position, cmd.getTransform().size, textMaterials);

            cmdIdx += style->text->size();
        }
        auto& drawContainer = pass.usingDrawContainer<ScissorDrawContainer>();

        ScissorDrawCommand draw;
        draw.geometry = widgetRenderer.getRectangleGeometry();
        draw.drawRange = gq.getFullDrawRange(draw.geometry);
        draw.drawRange.firstInstance = 0;
        draw.drawRange.instanceCount = numInstances;
        drawContainer.draw(draw);
    }
};