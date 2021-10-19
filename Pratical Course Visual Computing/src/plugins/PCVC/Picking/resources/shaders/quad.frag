#version 430

// --------------------------------------------------------------------------------
//  TODO: Complete this shader!
// --------------------------------------------------------------------------------

in vec2 texCoords;

layout(location = 0) out vec4 fragColor;

uniform int showFBOAtt;
uniform vec3 lightPos; // Light position
uniform sampler2D fboTexColor;
uniform sampler2D fboTexId;
uniform sampler2D fboTexNormals;
uniform sampler2D fboTexDepth;

// Linearize depth buffer values
float linearizeDepth(vec2 uv) {
    float zNear = 0.01f;
    float zFar = 20.0;
    float z = texture(fboTexDepth, uv).x;
    return (2.0 * zNear) / (zFar + zNear - z * (zFar - zNear));
}

void main() {
    vec4 color;

    float Ia = 1.0;
    float Iin = 1.0;
    float kamb = 0.2;
    float kdiff = 0.8;
    float kspec = 0.0;

    float Iamb = kamb * Ia;
    float Idiff = kdiff * Iin;
    float Ispec = 0.0;

    // showFBOAtt: ID
    if (showFBOAtt == 1) color = texture(fboTexId, texCoords);
    // showFBOAtt: Normals
    else if(showFBOAtt == 2) color = texture(fboTexNormals, texCoords);
    // showFBOAtt: Depth
    else if(showFBOAtt == 3) {
        float depth = linearizeDepth(texCoords);
        color = vec4(depth, depth, depth, 1.0);
    // showFBOAtt: Deferred
    } else if(showFBOAtt == 6) {
        vec4 N = normalize( texture(fboTexNormals, texCoords) );
        vec4 L = normalize( vec4(lightPos, 1.0) );
        color = Idiff * max(dot(N, L), 0.0) * texture(fboTexColor, texCoords);
    }
    // showFBOAtt: Color and others
    else color = texture(fboTexColor, texCoords);

    fragColor = color;
}
