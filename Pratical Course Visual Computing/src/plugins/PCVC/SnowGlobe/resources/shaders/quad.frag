#version 430

#define M_PI 3.14159265358979323846

in vec2 texCoords;

layout(location = 0) out vec4 fragColor;

uniform int showFBOAtt;
uniform vec3 lightPos; // Light position
uniform vec3 cameraPos; // Camera position
uniform mat4 invViewProjMx;

uniform sampler2D fboTexColor;
uniform sampler2D fboTexId;
uniform sampler2D fboTexNormals;
uniform sampler2D fboTexPos;
uniform sampler2D fboTexDepth;

uniform vec3 ambient;                  //!< ambient color
uniform vec3 diffuse;                  //!< diffuse color
uniform vec3 specular;                 //!< specular color

uniform float k_amb;                   //!< ambient factor
uniform float k_diff;                  //!< diffuse factor
uniform float k_spec;                  //!< specular factor
uniform float k_exp;                   //!< specular exponent

// Linearize depth buffer values
float linearizeDepth(vec2 uv) {
    float zNear = 0.01f;
    float zFar = 20.0;
    float z = texture(fboTexDepth, uv).x;
    return (2.0 * zNear) / (zFar + zNear - z * (zFar - zNear));
}

// Traansform coordinates from texture to world
vec3 coordTransform(vec2 uv, float depth){
    vec4 ndcPos = vec4(2.0f * uv.x - 1.0f, 2.0f * uv.y - 1.0, -1.0f, 1.0f);
    vec4 worldPos = invViewProjMx * ndcPos;

    return vec3(worldPos.xy, depth);
}

// Blinn-Phong shading
vec3 blinnPhong(vec3 n, vec3 l, vec3 v) {
    vec3 h = normalize(v + l);

    vec3 ambientLight = k_amb * ambient;
    vec3 diffuseLight = k_diff * diffuse * max(0.0, dot(n, l));
    vec3 specularLight = k_spec * specular * pow( max(0.0, dot(n, h)), k_exp) * (k_exp + 2) / (2 * M_PI);
    vec3 color = ambientLight + diffuseLight + specularLight;

    return color;
}

void main() {
    vec4 color;

    if (showFBOAtt == 1) { // showFBOAtt: ID
        color = texture(fboTexId, texCoords);
    }
    else if (showFBOAtt == 2) { // showFBOAtt: Normals
        color = texture(fboTexNormals, texCoords);
    }
    else if (showFBOAtt == 3) { // showFBOAtt: Position
        color = texture(fboTexPos, texCoords);
    }
    else if (showFBOAtt == 4) { // showFBOAtt: Depth
        float depth = linearizeDepth(texCoords);
        color = vec4(depth, depth, depth, 1.0);
    }
    else if (showFBOAtt == 5) { // showFBOAtt: Deferred
        vec4 sphereCheck = texture(fboTexPos, texCoords);
        if(distance(sphereCheck.xyz, vec3(0.0f)) < 2.8f){
            float depth = linearizeDepth(texCoords);
            //vec3 pos = texture(fboTexPos, texCoords).xyz;
            vec3 pos = coordTransform(texCoords, depth);

            vec3 N = normalize(texture(fboTexNormals, texCoords).xyz);
            vec3 L = normalize(lightPos - pos);
            vec3 V = normalize(cameraPos - pos);
            vec4 C = texture(fboTexColor, texCoords);

            color = vec4(blinnPhong(N, L, V) * C.xyz, C.w);
        } else color = texture(fboTexColor, texCoords);
    }
    else color = texture(fboTexColor, texCoords); // showFBOAtt: Color

    fragColor = color;
}
