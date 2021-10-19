#version 430

uniform samplerCube tex;
uniform float domeAlpha;
uniform vec3 cameraPos;

in vec3 position;
in vec3 normal;

layout(location = 0) out vec4 fragColor0; // Color
layout(location = 2) out vec4 fragColor2; // Normal

void main() {
    // Calculate the coordinates of reflection and refraction
    float ratio = 1.00 / 1.52;
    vec3 I = normalize(position - cameraPos);

    vec3 reflectPos = reflect(I, normal);
    vec3 refractPos = refract(I, -normal, ratio);
    float refFactor = dot(normal, I);

    vec3 reflectColor = vec3(texture(tex, reflectPos));
    vec3 refractColor = vec3(texture(tex, refractPos));

    fragColor0 = vec4(mix(refractColor, reflectColor, refFactor), domeAlpha);
    fragColor2 = vec4(normal, domeAlpha);
}
