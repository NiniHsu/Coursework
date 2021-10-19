#version 430

// --------------------------------------------------------------------------------
//  TODO: Complete this shader!
// --------------------------------------------------------------------------------

in vec4 vertexColor;
in vec2 textureCoord;

out vec4 fragColor;

uniform int patternFreq;
uniform int showTextureInt;
uniform sampler2D textureSampler;

void main() {
    // If the texture checkbox is checked
    if (showTextureInt == 1){ // Texture Mapping
        fragColor = texture(textureSampler, textureCoord);
    // If not, execute procedural shader
    } else { // Color shading
        fragColor = vec4(0.5f, 0.5f, 0.5f, 0.0f) + vertexColor;
    }

    // If the 3D checkerboard checkbox is checked
    if(patternFreq > 0){ // 3D checkerboard
        int num = patternFreq + 1;
        float integer;
        float chessboard = floor((vertexColor.x + 0.5f) * num + 0.5f) + floor((vertexColor.y + 0.5f) * num + 0.5f) + floor((vertexColor.z + 0.5f) * num + 0.5f);
        chessboard = modf(chessboard * 0.5, integer);
        chessboard *= 2;
        if(chessboard == 0.0f){
            fragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}
