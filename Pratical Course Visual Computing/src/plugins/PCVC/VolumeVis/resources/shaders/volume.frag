#version 430

#define M_PI 3.14159265358979323846

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38

uniform sampler3D volumeTex;           //!< 3D texture handle
uniform sampler1D transferTex;

uniform mat4 invViewMx;                //!< inverse view matrix
uniform mat4 invViewProjMx;            //!< inverse view-projection matrix

uniform vec3 volumeRes;                //!< volume resolution
uniform vec3 volumeDim;                //!< volume dimensions

uniform int viewMode; //<! rendering method: 0: line-of-sight, 1: mip, 2: isosurface, 3: volume
uniform bool showBox;
uniform bool useRandom;

uniform int maxSteps;                  //!< maximum number of steps
uniform float stepSize;                //!< step size
uniform float scale;                   //!< scaling factor

uniform float isovalue;                //!< value for iso surface

uniform vec3 ambient;                  //!< ambient color
uniform vec3 diffuse;                  //!< diffuse color
uniform vec3 specular;                 //!< specular color

uniform float k_amb;                   //!< ambient factor
uniform float k_diff;                  //!< diffuse factor
uniform float k_spec;                  //!< specular factor
uniform float k_exp;                   //!< specular exponent

in vec2 texCoords;

layout(location = 0) out vec4 fragColor;

struct Ray {
    vec3 o; // origin of the ray
    vec3 d; // direction of the ray
};


/**
 * Test for an intersection with the bounding box.
 * @param r             The ray to use for the intersection test
 * @param boxmin        The minimum point (corner) of the box
 * @param boxmax        The maximum point (corner) of the box
 * @param tnear[out]    The distance to the c plane intersection
 * @param tfar[out]     The distance to the farthest plane intersection
 */
bool intersectBox(Ray r, vec3 boxmin, vec3 boxmax, out float tnear, out float tfar) {
    // --------------------------------------------------------------------------------
    //  TODO: Calculate intersection between ray and box.
    // --------------------------------------------------------------------------------
    float temp;

    // With x-axis
    float tMin = (boxmin.x - r.o.x) / r.d.x;
    float tMax = (boxmax.x - r.o.x) / r.d.x;
    if (tMin > tMax) { temp = tMin; tMin = tMax; tMax = temp; }

    // With y-axis
    float tyMin = (boxmin.y - r.o.y) / r.d.y;
    float tyMax = (boxmax.y - r.o.y) / r.d.y;
    if (tyMin > tyMax) { temp = tyMin; tyMin = tyMax; tyMax = temp; }

    if ((tMin > tyMax) || (tyMin > tMax)) return false; 
    if (tyMin > tMin) tMin = tyMin; 
    if (tyMax < tMax) tMax = tyMax; 

    // With z-axis
    float tzMin = (boxmin.z - r.o.z) / r.d.z;
    float tzMax = (boxmax.z - r.o.z) / r.d.z;
    if (tzMin > tzMax) { temp = tzMin; tzMin = tzMax; tzMax = temp; }

    if ((tMin > tzMax) || (tzMin > tMax)) return false; 
    if (tzMin > tMin) tMin = tzMin; 
    if (tzMax < tMax) tMax = tzMax; 

    tnear = tMin;
    tfar = tMax;

    return true;
}

/**
 * Test if the given position is near a box edge.
 * @param pos           The position to test against
 */
bool isBoxEdge(vec3 pos) {
    // --------------------------------------------------------------------------------
    //  TODO: Check if the position is near an edge of the volume cube.
    // --------------------------------------------------------------------------------
    // Calculate the difference between the point and min/max value of the volume
    vec3 diffNear = abs(pos - 0.5 * volumeDim);
    vec3 diffFar = abs(pos + 0.5 * volumeDim);

    // Check the number of the difference which is less than the threshold
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (diffNear[i] < 0.01) count ++;
        if (diffFar[i] < 0.01) count ++;
    }

    // More than 2 differences < threshold represents it closes to the edge
    if (count >=2) return true;
    else return false;
}

/**
 * Map world coordinates to texture coordinates.
 * @param pos           The world coordinates to transform to texture coordinates
 */
vec3 mapTexCoords(vec3 pos) {
    // --------------------------------------------------------------------------------
    //  TODO: Map world pos to texture coordinates for 3D volume texture.
    // --------------------------------------------------------------------------------
    // Volume value is in [-0.5 * volumeDim, 0.5 * volumeDim]
    return pos / volumeDim + vec3(0.5);
}

/**
 * Calculate normals based on the volume gradient.
 */
vec3 calcNormal(vec3 pos) {
    // --------------------------------------------------------------------------------
    //  TODO: Calculate normals based on volume gradient.
    // --------------------------------------------------------------------------------
    vec3 volumeCoord = mapTexCoords(pos);
    vec3 gradient;
    gradient.x = textureOffset(volumeTex, volumeCoord, ivec3(1, 0, 0)).x - textureOffset(volumeTex, volumeCoord, ivec3(-1, 0, 0)).x;
    gradient.y = textureOffset(volumeTex, volumeCoord, ivec3(0, 1, 0)).x - textureOffset(volumeTex, volumeCoord, ivec3(0, -1, 0)).x;
    gradient.z = textureOffset(volumeTex, volumeCoord, ivec3(0, 0, 1)).x - textureOffset(volumeTex, volumeCoord, ivec3(0, 0, -1)).x;

    return normalize( gradient );
}

/**
 * Calculate the correct pixel color using the Blinn-Phong shading model.
 * @param n             The normal at this pixel
 * @param l             The direction vector towards the light
 * @param v             The direction vector towards the viewer
 */
vec3 blinnPhong(vec3 n, vec3 l, vec3 v) {
    vec3 color = vec3(0.0);
    // --------------------------------------------------------------------------------
    //  TODO: Calculate correct Blinn-Phong shading.
    // --------------------------------------------------------------------------------
    vec3 h = normalize(v + l);
    color += k_amb * ambient;
    color += k_diff * diffuse * max(0.0, dot(n, normalize(l)));
    color += k_spec * specular * pow( max(0.0, dot(n, h)), k_exp) * (k_exp + 2) / (2 * M_PI);

    return color;
}

/**
 * Pseudorandom function, returns a float value between 0.0 and 1.0.
 * @param seed          Positive integer that acts as a seed
 */
float random(uint seed) {
    // wang hash
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return float(seed) / 4294967296.0;
}

/**
 * The main function of the shader.
 */
void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    // --------------------------------------------------------------------------------
    //  TODO: Set up the ray and box. Do the intersection test and draw the box.
    // --------------------------------------------------------------------------------
    // Coordinate Transform
    float NDCPosX = 2.0f * texCoords.x - 1.0f;
    float NDCPosY = 2.0f * texCoords.y - 1.0f;
    vec4 clipPos = vec4(NDCPosX, NDCPosY, -1.0, 1.0);

    // Construct ray 
    struct Ray ray;
    ray.o = (invViewMx * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    ray.d = normalize( (invViewProjMx * clipPos).xyz - ray.o);

    // Check the interscetion
    float tNear, tFar;
    bool isIntersect = intersectBox(ray, -0.5 * volumeDim, 0.5 * volumeDim, tNear, tFar);
    if(!isIntersect) discard;
    vec3 posNear = tNear * ray.d + ray.o; // Position of intersection on the closest plane
    vec3 posFar = tFar * ray.d + ray.o; // Position of intersection on the farthest plane

    // Check whether the intersections are on the bounding box edge
    bool isFrontFaceEdge = false;
    bool isBackFaceEdge = false;
    if (isBoxEdge(posNear)) isFrontFaceEdge = true;
    if (isBoxEdge(posFar)) isBackFaceEdge = true;

    // --------------------------------------------------------------------------------
    //  TODO: Draw the volume based on the current view mode.
    // --------------------------------------------------------------------------------
    switch (viewMode) {
        case 0: { // line-of-sight
            // --------------------------------------------------------------------------------
            //  TODO: Implement line of sight (LoS) rendering.
            // --------------------------------------------------------------------------------
            float value = 0.0f;

            for (int i = 1; i <= maxSteps; i++) {
                float tStep = stepSize * i + tNear;
                if (tStep >= tFar) break;

                vec3 samplePos = tStep * ray.d + ray.o;
                value += texture(volumeTex, mapTexCoords(samplePos)).x * scale;
            }
            color = vec4(value, value, value, 1.0);
            break;
        }
        case 1: { // maximum-intesity projection
            // --------------------------------------------------------------------------------
            //  TODO: Implement maximum intensity projection (MIP) rendering.
            // --------------------------------------------------------------------------------
            float value = 0.0f;

            for (int i = 1; i <= maxSteps; i++) {
                float tStep = stepSize * i + tNear;
                if (tStep >= tFar) break;

                vec3 samplePos = tStep * ray.d + ray.o;
                float sampleValue = texture(volumeTex, mapTexCoords(samplePos)).x;
                if (sampleValue > value) value = sampleValue;
            }
            color = vec4(value, value, value, 1.0);
            break;
        }
        case 2: { // isosurface
            // --------------------------------------------------------------------------------
            //  TODO: Implement isosurface rendering.
            // --------------------------------------------------------------------------------
            float sampleLastValue = 0.0f;
            vec3 sampleLastPos = ray.o;

            for (int i = 1; i <= maxSteps; i++) {
                float tStep = stepSize * i + tNear;
                if (tStep >= tFar) break;

                vec3 samplePos = tStep * ray.d + ray.o;
                float sampleValue = texture(volumeTex, mapTexCoords(samplePos)).x;
                if (sampleLastValue > isovalue) {
                    // Calculate the position and the normal of isovalue, and use Blinn-Phong shading
                    vec3 iosvaluePos = mix(sampleLastPos, samplePos, (isovalue - sampleLastValue) / (sampleValue - sampleLastValue));
                    vec3 normal = calcNormal(iosvaluePos);
                    if(color != vec4(1.0, 1.0, 0.0, 1.0)) color = vec4(blinnPhong(-normal, ray.o, -ray.d), 1.0);
                    break;
                }
                sampleLastValue = sampleValue;
                sampleLastPos = samplePos;
            }
            // Discard the fragment if it's not on the edge and not an isovalue (volume is transparent)
            if(!isFrontFaceEdge && !isBackFaceEdge && (color == vec4(0.0, 0.0, 0.0, 1.0))) discard;
            break;
        }
        case 3: { // volume visualization with transfer function
            // --------------------------------------------------------------------------------
            //  TODO: Implement volume rendering.
            // --------------------------------------------------------------------------------
            break;
        }
        default: {
            color = vec4(1.0, 0.0, 0.0, 1.0);
            break;
        }
    }

    // --------------------------------------------------------------------------------
    //  TODO: Draw the box lines behind the volume, if the volume is transparent.
    // --------------------------------------------------------------------------------
    // Draw bounding box edge
    if (showBox) {
        if (isFrontFaceEdge) color = vec4(1.0, 1.0, 0.0, 1.0);
        if (isBackFaceEdge && (viewMode == 2) && color == vec4(0.0, 0.0, 0.0, 1.0)) color = vec4(1.0, 1.0, 0.0, 1.0);
    }

    fragColor = color;
}
