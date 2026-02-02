#version 460 core

layout(location = 0) out vec4 f_FragColor;
in vec4 color;

void main() {
    f_FragColor = color;
}