#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 v_TexCoords;

uniform sampler2D scene;

void main()
{
    vec4 color = texture(scene, v_TexCoords).rgba;

    FragColor = vec4(color);
}