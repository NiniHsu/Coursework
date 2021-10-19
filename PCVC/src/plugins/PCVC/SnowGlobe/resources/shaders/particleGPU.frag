#version 430

uniform sampler2D tex;

in vec2 texCoords;
in vec3 pos;

layout(location = 0) out vec4 fragColor0; // Color

void main() {
    vec4 texColor = texture(tex, texCoords);
    if(texColor.a < 0.1) discard;
    if(distance(pos, vec3(0.0)) >= 2.4) discard; // Discard if the particles are not in the dome
    fragColor0 = texColor;
}
