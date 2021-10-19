#version 430

uniform mat4 projMx;
uniform mat4 viewMx;

layout(location = 0) in vec3 in_position;

out vec3 texCoords;

void main() {
    vec4 pos = projMx * viewMx * vec4(in_position, 1.0);
    gl_Position = pos.xyww;
    texCoords = in_position;
}
