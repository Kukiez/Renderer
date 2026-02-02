#version 460 core
#extension GL_ARB_gpu_shader_int64 : enable

in vec2 v_LocalPosition;
in vec2 TexCoords;
in flat int instanceID;

out vec4 FragColor;

struct UITextMaterial {
    vec2 position;
    vec2 size;
    vec2 uvOffset;
    vec2 uvScale;
    vec4 color;
    uint texture;
    int mode;
    float borderThickness;
    uint font;
    vec4 charUV;
};


layout(std430, binding = 1) buffer UIInstances {
    UITextMaterial instances[];
};

layout(std430, binding = 2) buffer MaterialData {
    uint64_t materials[];
};

void main() {
    UITextMaterial instance = instances[instanceID];

    vec4 tint = instance.color;

    uint texID = instance.texture;
    vec2 uvOffset = instance.uvOffset;
    vec2 uvScale = instance.uvScale;
    int mode = instance.mode;
    vec2 size = instance.size;
    float thickness = instance.borderThickness;
    uint font = instance.font;

    vec2 charUVMin = instance.charUV.xy;
    vec2 charUVMax = instance.charUV.zw;

    vec2 uv = fract(TexCoords * uvScale + uvOffset);

    vec2 fontUV = mix(charUVMin, charUVMax, TexCoords);

    float coverage = texture(sampler2D(materials[font]), fontUV).r;

    vec4 imgColor = texture(sampler2D(materials[texID]), uv);

    FragColor = vec4(
        tint
    );
    FragColor.a *= coverage;
}
