#version 430

uniform mat4 projMx;
uniform mat4 viewMx;
uniform mat4 modelMx;
uniform mat3 normalMx;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texCoords;

out vec3 position;
out vec3 normal;

void main() {
    gl_Position = projMx * viewMx * modelMx * vec4(in_position, 1.0); 
    normal = normalize(normalMx * in_position);
    position = vec3(modelMx * vec4(in_position, 1.0));
}
