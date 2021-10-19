#version 430

// --------------------------------------------------------------------------------
//  TODO: Complete this shader!
// --------------------------------------------------------------------------------

uniform vec3 pickIdCol;
uniform sampler2D tex;

in vec3 normal;
in vec2 texCoords;

layout(location = 0) out vec4 fragColor0; // Color
layout(location = 1) out vec4 fragColor1; // ID
layout(location = 2) out vec4 fragColor2; // Normals

void main() {
    fragColor0 = texture(tex, vec2(texCoords.x, 1.0-texCoords.y));
    fragColor1 = vec4(pickIdCol, 1.0);
    fragColor2 = vec4(normal, 0.0);
}
