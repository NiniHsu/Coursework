#version 430

//  Set the tessellation mode
layout(quads, equal_spacing, cw) in;
/*layout (std430, binding = 0) readonly buffer knotVector {
   std::vector<float> knotVector_u;
   std::vector<float> knotVector_v;
};*/

uniform mat4 projMx;
uniform mat4 viewMx;

out vec2 texCoords;
out vec3 normal;

void main() {
    // Interpolate the position of the vertex in world coord.
    vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
    vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
    vec4 pos = mix(p1, p2, gl_TessCoord.y);
    gl_Position = projMx * viewMx * pos;
    texCoords = gl_TessCoord.xy;
    normal = normalize(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));
}
