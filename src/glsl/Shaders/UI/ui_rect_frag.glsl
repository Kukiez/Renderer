#version 460 core
#extension GL_ARB_gpu_shader_int64 : enable

in vec2 TexCoords;
in flat int instanceID;

out vec4 FragColor;

struct UIRectangleMaterial {
    vec2 position;
    vec2 size;
    vec2 uvMin;
    vec2 uvMax;
    vec4 color;
    uint texture;
    int mode;
    float borderThickness;
    int padding;
};

layout(std430, binding = 1) buffer UIInstances {
    UIRectangleMaterial instances[];
};

layout(std430, binding = 2) buffer MaterialData {
    uint64_t textures[];
};

const int MODE_FILL = 0;
const int MODE_BORDER = 1;
const int MODE_GLOW = 2;
const int MODE_SHADOW = 3;


in vec2 v_LocalPosition;

float signedDistRect(vec2 uv, vec2 sizePx)
{
    vec2 p = uv * sizePx;
    vec2 q = abs(p - sizePx * 0.5) - sizePx * 0.5;

    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0);
}

void main() {
    UIRectangleMaterial instance = instances[instanceID];

    vec4 tint = instance.color;

    uint texID = instance.texture;
    int mode = instance.mode;
    vec2 size = instance.size;
    float thickness = instance.borderThickness;

    vec2 uv = TexCoords;

    vec4 texColor = texture(sampler2D(textures[texID]), uv);

    switch (mode) {
        case MODE_FILL: {
            FragColor = texColor + tint;
            break;
        }
        case MODE_BORDER: {
            float left   = v_LocalPosition.x * size.x;
            float right  = (1.0 - v_LocalPosition.x) * size.x;
            float bottom = v_LocalPosition.y * size.y;
            float top    = (1.0 - v_LocalPosition.y) * size.y;

            float distToEdge = min(min(left, right), min(bottom, top));
            bool isBorder = distToEdge < instance.borderThickness;

            if (isBorder) {
                FragColor = texColor + tint;
            } else {
                FragColor = vec4(0);
            }
            break;
        }
        case MODE_GLOW: {
            float sd = signedDistRect(v_LocalPosition, size);

            float glow = smoothstep(
            thickness,
            0.0,
            -sd
            );
            glow = pow(glow, 2.0);
            FragColor = texColor + tint;
            FragColor *= glow;
            break;
        }
        case MODE_SHADOW: {
            float shadowSd = signedDistRect(uv, size);

            float shadowAlpha = smoothstep(
            thickness,
            0.0,
            shadowSd
            );
            FragColor = texColor + tint;
            FragColor *= 1 - shadowAlpha;
            break;
        }
        default: FragColor = vec4(0, 0, 0, 1);
    }
}