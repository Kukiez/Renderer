#pragma once
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Resource/Shader/ShaderKey.h>

#include "RendererAPI.h"

struct BloomResourceSystem {
    ShaderKey downsampleShader;
    ShaderKey upsampleShader;
    ShaderKey bloomExtractShader;
    ShaderKey applyBloomShader;

    RENDERERAPI void onLoad(RendererLoadView renderer);
};
