#version 430

uniform samplerCube tex;

in vec3 texCoords;

layout(location = 0) out vec4 fragColor0; // Color

void main() {
    fragColor0 = texture(tex, texCoords);
}
