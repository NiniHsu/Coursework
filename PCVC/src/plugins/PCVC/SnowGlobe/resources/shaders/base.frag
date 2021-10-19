#version 430

uniform vec3 pickIdCol;
uniform sampler2D tex;

in vec3 normal;
in vec2 texCoords;
in vec3 pos;

layout(location = 0) out vec4 fragColor0; // Color
layout(location = 1) out vec4 fragColor1; // ID
layout(location = 2) out vec4 fragColor2; // Normals
layout(location = 3) out vec4 fragColor3; // Position

void main() {
    if(distance(texCoords, vec2(0.5, 0.5)) > 0.5) discard;
    fragColor0 = texture(tex, texCoords);
    fragColor1 = vec4(pickIdCol, 1.0);
    fragColor2 = vec4(normal, 1.0);
    fragColor3 = vec4(pos, 1.0);
}
