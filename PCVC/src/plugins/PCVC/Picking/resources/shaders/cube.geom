#version 430

// --------------------------------------------------------------------------------
//  TODO: Complete this shader!
// --------------------------------------------------------------------------------

uniform mat4 projMx;
uniform mat4 viewMx;
uniform mat4 modelMx;
uniform mat3 normalMx;

layout(points) in;
layout(triangle_strip, max_vertices=25) out;

out vec3 normal;
out vec2 texCoords;

vec4 transform(vec4 pos) {
    return projMx * viewMx * modelMx * pos;
}

void main() {
    vec4 origin = gl_in[0].gl_Position;

    // Face two
    normal = normalize(normalMx * vec3(-0.5, 0.0, 0.0));

    gl_Position = transform(origin + vec4(-0.5, 0.5, 0.5, 0.0)); // 0
    texCoords = vec2(0.0, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, -0.5, 0.5, 0.0)); // 1
    texCoords = vec2(0.0, 0.5);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, 0.5, -0.5, 0.0)); // 2
    texCoords = vec2(0.25, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, -0.5, -0.5, 0.0)); // 3
    texCoords = vec2(0.25, 0.5);
    EmitVertex();

    //Face four
    normal = normalize(normalMx * vec3(0.0, 0.0, -0.5));

    gl_Position = transform(origin + vec4(-0.5, 0.5, -0.5, 0.0)); // 2
    texCoords = vec2(0.25, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, -0.5, -0.5, 0.0)); // 3
    texCoords = vec2(0.25, 0.5);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, 0.5, -0.5, 0.0)); // 4
    texCoords = vec2(0.5, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, -0.5, 0.0)); // 5
    texCoords = vec2(0.5, 0.5);
    EmitVertex();

    // Face five
    normal = normalize(normalMx * vec3(0.5, 0.0, 0.0));

    gl_Position = transform(origin + vec4(0.5, 0.5, -0.5, 0.0)); // 4
    texCoords = vec2(0.5, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, -0.5, 0.0)); // 5
    texCoords = vec2(0.5, 0.5);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, 0.5, 0.5, 0.0)); // 6
    texCoords = vec2(0.75, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, 0.5, 0.0)); // 7
    texCoords = vec2(0.75, 0.5);
    EmitVertex();

    // Face three
    normal = normalize(normalMx * vec3(0.0, 0.0, 0.5));

    gl_Position = transform(origin + vec4(0.5, 0.5, 0.5, 0.0)); // 6
    texCoords = vec2(0.75, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, 0.5, 0.0)); // 7
    texCoords = vec2(0.75, 0.5);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, 0.5, 0.5, 0.0)); // 8
    texCoords = vec2(1.0, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, -0.5, 0.5, 0.0)); // 9
    texCoords = vec2(1.0, 0.5);
    EmitVertex();

    // Face six
    normal = normalize(normalMx * vec3(0.0, -0.5, 0.0));

    gl_Position = transform(origin + vec4(-0.5, -0.5, 0.5, 0.0)); // 9
    texCoords = vec2(0.25, 0.25);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, -0.5, -0.5, 0.0)); // 10
    texCoords = vec2(0.25, 0.5);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, 0.5, 0.0)); // 11
    texCoords = vec2(0.5, 0.25);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, -0.5, 0.0)); // 12
    texCoords = vec2(0.5, 0.5);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, -0.5, -0.5, 0.0)); // 12
    texCoords = vec2(0.5, 0.5);
    EmitVertex();

    // Face one
    normal = normalize(normalMx * vec3(0.0, 0.5, 0.0));

    gl_Position = transform(origin + vec4(0.5, 0.5, -0.5, 0.0)); // 13
    texCoords = vec2(0.5, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(0.5, 0.5, 0.5, 0.0)); // 14
    texCoords = vec2(0.5, 1.0);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, 0.5, -0.5, 0.0)); // 15
    texCoords = vec2(0.25, 0.75);
    EmitVertex();
    gl_Position = transform(origin + vec4(-0.5, 0.5, 0.5, 0.0)); // 16
    texCoords = vec2(0.25, 1.0);
    EmitVertex();

    EndPrimitive();  
}

