#version 430

uniform mat4 projMx;
uniform mat4 viewMx;

layout(location = 0) in vec3 in_vertex_position;
layout(location = 1) in vec4 in_particle_position;

out vec2 texCoords;

void main() {
    float life = in_particle_position.w;

    // Select texture depends on lifetime
    texCoords = in_vertex_position.xy / 2;      // Left top texture
    if ((life > 0.5) && (life <= 0.75)) texCoords.x += 0.5f;      // Right top texture
    else if ((life > 0.25) && (life <= 0.5)) texCoords.y += 0.5f;      // Left bottom texture
    else if ((life > 0.0) && (life <= 0.25)) texCoords += vec2(0.5f);   // Right bottom texture

    vec4 position_viewspace = viewMx * vec4(in_particle_position.xyz , 1.0);
    position_viewspace.xy += 0.05 * (in_vertex_position.xy - vec2(0.5)); // Scale the snowflake
    gl_Position = projMx * position_viewspace;
}
