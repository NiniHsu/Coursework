#include "Objects.h"

#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

#include "Picking.h"

using namespace OGL4Core2::Plugins::PCVC::Picking;

static const double pi = std::acos(-1.0);

/**
 * Calculates a unique color from an ID.
 * @param id       ID to use for the calculation
 * @returns        unique color as glm::vec3
 */
glm::vec3 Object::idToColor(unsigned int id) {
    glm::vec3 col(0.0f);
    // --------------------------------------------------------------------------------
    //  TODO: Implement id to color transformation!
    //        Make sure that this works for a variable number of objects.
    // --------------------------------------------------------------------------------
    // Pick the least signifying bits to red channel and the most significant bits to blue channel
    int r = (id & 0x000000FF) >> 0;
    int g = (id & 0x0000FF00) >> 8;
    int b = (id & 0x00FF0000) >> 16;
    col = glm::vec4(r / 255.0, g / 255.0, b / 255.0, 1.0);
    return col;
}

/**
 * Calculates the correct ID from a unique color.
 * @param col      color to use for the calculation
 */
unsigned int Object::colorToId(const unsigned char col[3]) {
    unsigned int id = 0;
    // --------------------------------------------------------------------------------
    //  TODO: Implement color to id transformation!
    // --------------------------------------------------------------------------------
    id = col[0] + col[1] * 256 + col[2] * 256 * 256;
    return id;
}

Object::Object(Picking& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex)
    : modelMx(glm::mat4(1.0f)),
      basePlugin(basePlugin),
      id(id),
      idCol(glm::vec3(0.0f)),
      shaderProgram(nullptr),
      va(nullptr),
      tex(std::move(tex)) {
    idCol = idToColor(id);
}

/**
 * The object is rendered.
 * @param projMx   projection matrix
 * @param viewMx   view matrix
 */
void Object::draw(const glm::mat4& projMx, const glm::mat4& viewMx) {
    if (shaderProgram == nullptr || va == nullptr) {
        return;
    }

    // --------------------------------------------------------------------------------
    //  TODO Implement object rendering!
    // --------------------------------------------------------------------------------

    shaderProgram->use();
    va->bindVertexArray();

    // Set matrix and pickIdCol as uniform
    shaderProgram->setUniform("modelMx", modelMx);
    shaderProgram->setUniform("projMx", projMx);
    shaderProgram->setUniform("viewMx", viewMx);
    glm::mat3 normalMx = glm::mat3(transpose(inverse(modelMx)));
    shaderProgram->setUniform("normalMx", normalMx);
    shaderProgram->setUniform("pickIdCol", idCol);

    // Set texture as uniform
    if (tex != nullptr) {
        glActiveTexture(GL_TEXTURE0);
        tex->bindTexture();
        shaderProgram->setUniform("tex", 0);
    }
    
    va->draw();

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Base::Base(Picking& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex)
    : Object(basePlugin, id, std::move(tex)) {
    initShaders();

    const std::vector<float> baseVertices{
        // clang-format off
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        // clang-format on
    };

    const std::vector<float> baseNormals{
        // clang-format off
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        // clang-format on
    };

    const std::vector<float> baseTexCoords{
        // clang-format off
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        // clang-format on
    };

    const std::vector<GLuint> baseIndices{0, 1, 2, 3};

    glowl::VertexLayout baseLayout{
        {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {3, GL_FLOAT, GL_FALSE, 0}, {2, GL_FLOAT, GL_FALSE, 0}}};
    va = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{baseVertices, baseNormals, baseTexCoords},
                                       baseIndices, baseLayout, GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLE_STRIP);
}

/**
 * This function is caled from the plugin after pressing 'R' to reload/recompile shaders.
 */
void Base::reloadShaders() {
    initShaders();
}

/**
 * Creates the shader program for this object.
 */
void Base::initShaders() {
    try {
        shaderProgram = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, basePlugin.getStringResource("shaders/base.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, basePlugin.getStringResource("shaders/base.frag")}});
    } catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

/**
 * Constructs a new cube instance.
 * @param basePlugin A reference to your Picking plugin
 * @param id         Unique ID to use for the new cube instance
 * @param tex        Texture for the new cube instance
 */
Cube::Cube(Picking& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex)
    : Object(basePlugin, id, std::move(tex)) {
    initShaders();

    // --------------------------------------------------------------------------------
    //  TODO: Init cube vertex data!
    // --------------------------------------------------------------------------------
    // Cube geometry is constructed in geometry shader, therefore only a single point vertex is specified here.
    const std::vector<float> cubeVertices{0.0f, 0.0f, 0.0f};
    const std::vector<GLuint> cubeIndices{0};

    glowl::VertexLayout cubeLayout{
        {0}, {{3, GL_FLOAT, GL_FALSE, 0}}};
    va = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{cubeVertices},
        cubeIndices, cubeLayout, GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_POINTS);

}

void Cube::reloadShaders() {
    initShaders();
}

void Cube::initShaders() {
    // --------------------------------------------------------------------------------
    //  TODO: Init cube shader program!
    // --------------------------------------------------------------------------------
    try {
        shaderProgram = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, basePlugin.getStringResource("shaders/cube.vert")},
            {glowl::GLSLProgram::ShaderType::Geometry, basePlugin.getStringResource("shaders/cube.geom")},
            {glowl::GLSLProgram::ShaderType::Fragment, basePlugin.getStringResource("shaders/cube.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

/**
 * Constructs a new sphere instance.
 * @param basePlugin A reference to your Picking plugin
 * @param id         Unique ID to use for the new sphere instance
 * @param tex        Texture for the new sphere instance
 */
Sphere::Sphere(Picking& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex)
    : Object(basePlugin, id, std::move(tex)) {
    initShaders();

    // --------------------------------------------------------------------------------
    //  TODO: Init sphere vertex data.
    // --------------------------------------------------------------------------------
    const int resTheta = 128;
    const int resPhi = 64;
    const double radiusSphere = 0.5;

    std::vector<float> sphereVertices;
    std::vector<float> sphereTexCoords;
    std::vector<GLuint> sphereIndices;

    // Create sphere vertices and texture coordinates
    const float stackStep = pi / float(resPhi);
    const float sectorStep = 2 * pi / float(resTheta);

    for (int i = 0; i <= resPhi; i++) {
        float currentStack = pi / 2.0 - stackStep * float(i);
        for (int j = 0; j <= resTheta; j++) {
            float currentSector = j * sectorStep;
            // Vertex
            sphereVertices.emplace_back( radiusSphere * cos(currentStack) * cos(currentSector) ); // vertex.x
            sphereVertices.emplace_back( radiusSphere * cos(currentStack) * sin(currentSector) ); // vertex.y
            sphereVertices.emplace_back( radiusSphere * sin(currentStack) ); // vertex.z
            // Texture coordinate
            sphereTexCoords.emplace_back( float(j) / resTheta ); // textcoord.u
            sphereTexCoords.emplace_back( float(i) / resPhi ); // textcoord.v
        }
    }

    // Create indices
    for (int i = 0; i < resPhi; i++) {
        GLuint currentVertexOffset = i * (resTheta + 1);
        GLuint nextVertexOffset = currentVertexOffset + resTheta + 1;
        for (int j = 0; j < resTheta; j++, currentVertexOffset++, nextVertexOffset++) {
            if (i != 0) {
                sphereIndices.emplace_back(currentVertexOffset);
                sphereIndices.emplace_back(nextVertexOffset);
                sphereIndices.emplace_back(currentVertexOffset + 1);
            }
            if (i != (resPhi)) {
                sphereIndices.emplace_back(currentVertexOffset + 1);
                sphereIndices.emplace_back(nextVertexOffset);
                sphereIndices.emplace_back(nextVertexOffset + 1);
            }
        }
    }


    glowl::VertexLayout sphereLayout{
    {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {2, GL_FLOAT, GL_FALSE, 0}} };
    va = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{sphereVertices, sphereTexCoords},
        sphereIndices, sphereLayout, GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLE_STRIP);
}

void Sphere::reloadShaders() {
    initShaders();
}

void Sphere::initShaders() {
    // --------------------------------------------------------------------------------
    //  TODO: Init sphere shader program!
    // --------------------------------------------------------------------------------
    try {
        shaderProgram = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, basePlugin.getStringResource("shaders/sphere.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, basePlugin.getStringResource("shaders/sphere.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

/**
 * Constructs a new torus instance.
 * @param basePlugin A reference to your Picking plugin
 * @param id         Unique ID to use for the new torus instance
 * @param tex        Texture for the new torus instance
 */
Torus::Torus(Picking& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex)
    : Object(basePlugin, id, std::move(tex)) {
    initShaders();

    // --------------------------------------------------------------------------------
    //  TODO: Init torus vertex data.
    // --------------------------------------------------------------------------------
    const int resT = 64; // Toroidal
    const int resP = 64; // Poloidal
    const float radiusOut = 0.34f;
    const float radiusIn = 0.16f;
    const GLuint numVertex = (resT + 1) * (resP + 1);

    std::vector<float> torusVertices;
    std::vector<float> torusNormals;
    std::vector<float> torusTexCoords;
    std::vector<GLuint> torusIndices;

    // Sample torus vertices and calculate normals for each vertex
    const float stepT = 2*pi / float(resT);
    const float stepP = 2*pi / float(resP);
    float currentT = 0.0f;

    for (int i = 0; i <= resT; i++) {
        float sinT = sin(currentT);
        float cosT = cos(currentT);
        float currentP = 0.0f;

        for (int j = 0; j <= resP; j++) {
            float sinP = sin(currentP);
            float cosP = cos(currentP);
            // Vertex
            torusVertices.emplace_back( (radiusOut + radiusIn * cosP) * cosT ); // vertex.x
            torusVertices.emplace_back( (radiusOut + radiusIn * cosP) * sinT ); // vertex.y
            torusVertices.emplace_back( radiusIn * sinP); // vertex.z
            // Normal
            torusNormals.emplace_back( cosT * cosP ); // normal.x
            torusNormals.emplace_back( sinT * cosP ); // normal.y
            torusNormals.emplace_back( sinP ); // normal.z

            currentP += stepP;
        }
        currentT += stepT;
    }

    // Calculate texture coordinates
    const float textureStepT = 1.0 / float(resT);
    const float textureStepP = 1.0 / float(resP);
    float textureCurrentT = 0.0f;

    for (int i = 0; i <= resT; i++) {
        float textureCurrentP = 0.0f;
        for (int j = 0; j <= resP; j++) {
            // Texture
            torusTexCoords.emplace_back(textureCurrentP); // texture.u
            torusTexCoords.emplace_back(textureCurrentT); // texture.v
            textureCurrentP += textureStepP;
        }
        textureCurrentT += textureStepT;
    }

    // Create indices
    GLuint currentVertexOffset = 0;

    for (int i = 0; i < resT; i++) {
        for (int j = 0; j <= resP; j++) {
            torusIndices.emplace_back(currentVertexOffset);
            torusIndices.emplace_back(currentVertexOffset + resP + 1);
            currentVertexOffset++;
        }
    }

    glowl::VertexLayout torusLayout{
        {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {3, GL_FLOAT, GL_FALSE, 0}, {2, GL_FLOAT, GL_FALSE, 0}} };
    va = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{torusVertices, torusNormals, torusTexCoords},
        torusIndices, torusLayout, GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLE_STRIP);
}

void Torus::reloadShaders() {
    initShaders();
}

void Torus::initShaders() {
    // --------------------------------------------------------------------------------
    //  TODO: Init torus shader program!
    // --------------------------------------------------------------------------------
    try {
        shaderProgram = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, basePlugin.getStringResource("shaders/torus.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, basePlugin.getStringResource("shaders/torus.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}
