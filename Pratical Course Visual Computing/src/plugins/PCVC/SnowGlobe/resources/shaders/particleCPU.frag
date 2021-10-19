#version 430

uniform sampler2D tex;

in vec2 texCoords;

layout(location = 0) out vec4 fragColor0; // Color

void main() {
    vec4 texColor = texture(tex, texCoords);
    if(texColor.a < 0.1) discard;
    fragColor0 = texColor;
}
