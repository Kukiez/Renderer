#pragma once
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Resource/Shader/ShaderKey.h>

struct BloomResourceSystem {
    ShaderKey downsampleShader;
    ShaderKey upsampleShader;
    ShaderKey bloomExtractShader;
    ShaderKey applyBloomShader;

    void onLoad(RendererLoadView renderer);
};
