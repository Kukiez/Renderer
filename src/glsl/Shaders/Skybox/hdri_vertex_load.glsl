#version 460 core

layout(location = 0) in vec3 aPos;

out vec3 v_WorldPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    v_WorldPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}