#version 430

uniform bool showNormals;
uniform int freq;

layout(location = 0) out vec4 fragColor0;
layout(location = 1) out vec4 fragColor1;

in vec2 texCoords;
in vec3 normal;

void main() {
    if (showNormals) fragColor0 = vec4(normal, 1.0f);
    else {
        //fragColor0 = vec4(texCoords, 0.0f, 1.0f);

        // Set up Checkerboard as Color
        int num = freq;
        float integer;
        float chessboard = floor(texCoords.x * num) + floor(texCoords.y * num);
        chessboard = modf(chessboard * 0.5, integer);
        chessboard *= 2;
        if(chessboard == 0.0f) fragColor0 = vec4(0.5f, 0.5f, 0.5f, 1.0f);
        else fragColor0 = vec4(0.8f, 0.8f, 0.8f, 0.8f);
    }
    fragColor1 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
