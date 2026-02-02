#version 460 core

uniform sampler2D srcTexture;
uniform int mipLevel;
uniform float filterRadius;

in vec2 v_TexCoords;
layout (location = 0) out vec4 upsample;

void main()
{
    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    float x = filterRadius;
    float y = filterRadius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===

    vec3 a = textureLod(srcTexture, vec2(v_TexCoords.x - x, v_TexCoords.y + y), mipLevel).rgb;
    vec3 b = textureLod(srcTexture, vec2(v_TexCoords.x,     v_TexCoords.y + y), mipLevel).rgb;
    vec3 c = textureLod(srcTexture, vec2(v_TexCoords.x + x, v_TexCoords.y + y), mipLevel).rgb;
    vec3 d = textureLod(srcTexture, vec2(v_TexCoords.x - x, v_TexCoords.y), mipLevel).rgb;
    vec3 e = textureLod(srcTexture, vec2(v_TexCoords.x,     v_TexCoords.y), mipLevel).rgb;
    vec3 f = textureLod(srcTexture, vec2(v_TexCoords.x + x, v_TexCoords.y), mipLevel).rgb;
    vec3 g = textureLod(srcTexture, vec2(v_TexCoords.x - x, v_TexCoords.y - y), mipLevel).rgb;
    vec3 h = textureLod(srcTexture, vec2(v_TexCoords.x,     v_TexCoords.y - y), mipLevel).rgb;
    vec3 i = textureLod(srcTexture, vec2(v_TexCoords.x + x, v_TexCoords.y - y), mipLevel).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    vec3 result = vec3(0);
    result = e*4.0;
    result += (b+d+f+h)*2.0;
    result += (a+c+g+i);
    result *= 1.0 / 16.0;

    upsample = vec4(result, 1.0f);
}