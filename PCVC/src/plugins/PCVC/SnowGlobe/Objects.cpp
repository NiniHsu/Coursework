#include "Objects.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "SnowGlobe.h"
#include "core/core.h"

using namespace OGL4Core2::Plugins::PCVC::SnowGlobe;

static const double pi = std::acos(-1.0);

/**
 * Calculates a unique color from an ID.
 * @param id       ID to use for the calculation
 * @returns        unique color as glm::vec3
 */
glm::vec3 Object::idToColor(unsigned int id) {
    glm::vec3 col(0.0f);
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
    id = col[0] + col[1] * 256 + col[2] * 256 * 256;
    return id;
}

Object::Object(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath)
    : modelMx(glm::mat4(1.0f)),
      basePlugin(basePlugin),
      id(id),
      idCol(glm::vec3(0.0f)),
      shaderProgram(nullptr),
      va(nullptr),
      tex(std::move(tex)),
      filepath(filepath){
    idCol = idToColor(id);
}

/**
 * The object is loaded.
 * @param filename   file which is going to be loaded
 */
void Object::loadObj(const std::string& filepath, std::unique_ptr<glowl::Mesh> &va) {
    std::cout << "Load model: " << filepath << std::endl;

    std::ifstream file(filepath, std::ifstream::in);
    if (!file) {
        std::cerr << "Cannot open " << filepath << std::endl;
        exit(1);
    }

    // Load model
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<GLuint> vertexIndices;
    std::vector<GLuint> normalIndices;
    std::vector<GLuint> texIndices;
    std::string token;

    while (file >> token) {
        if (token == "v") {
            float x, y, z;
            file >> x >> y >> z;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        } else if (token == "vn") {
            float x, y, z;
            file >> x >> y >> z;
            normals.push_back(x);
            normals.push_back(y);
            normals.push_back(z);
        } else if (token == "vt") {
            float x, y, z;
            file >> x >> y >> z;
            texCoords.push_back(x);
            texCoords.push_back(y);
        } else if (token == "f") { // f introduces a face, using vertex indices, starting at 1
            std::vector<int> v4;
            std::vector<int> n4;
            std::vector<int> t4;
            std::vector<int> i4{ 0, 1, 2, 2, 3, 0 };

            for (int i = 0; i < 4; i++) {
                std::string indexstr;
                file >> indexstr;
                std::size_t slash1 = indexstr.find("/");
                std::size_t slash2 = indexstr.find("/", slash1 + 1);
                v4.push_back(stoi(indexstr.substr(0, slash1)) - 1);
                t4.push_back(stoi(indexstr.substr(slash1 + 1, slash2)) - 1);
                n4.push_back(stoi(indexstr.substr(slash2 + 1)) - 1);
            }
            for (int i = 0; i < i4.size(); i++) {
                vertexIndices.push_back(v4[i4[i]]);
                normalIndices.push_back(n4[i4[i]]);
                texIndices.push_back(t4[i4[i]]);
            }
        }
    }

    file.close();

    // Modify the order of normals and indices
    std::vector<float> verticesReorder;
    std::vector<float> normalsReorder;
    std::vector<float> texCoordsReorder;
    std::vector<int> indicesReorder;
    for (int i = 0; i < normalIndices.size(); i++) {
        indicesReorder.push_back(i);
        int j = vertexIndices[i] * 3;
        verticesReorder.push_back(vertices[j]);
        verticesReorder.push_back(vertices[j + 1]);
        verticesReorder.push_back(vertices[j + 2]);
        int k = normalIndices[i] * 3;
        normalsReorder.push_back(normals[k]);
        normalsReorder.push_back(normals[k + 1]);
        normalsReorder.push_back(normals[k + 2]);
        int l = texIndices[i] * 2;
        texCoordsReorder.push_back(texCoords[l]);
        texCoordsReorder.push_back(texCoords[l + 1]);
    }

    glowl::VertexLayout vertexLayout{
        {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {3, GL_FLOAT, GL_FALSE, 0}, {2, GL_FLOAT, GL_FALSE, 0}} };
    va = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{verticesReorder, normalsReorder, texCoordsReorder}, indicesReorder, vertexLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLES);
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

Base::Base(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath)
    : Object(basePlugin, id, std::move(tex), filepath) {
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
 * Constructs a new sphere instance.
 * @param basePlugin A reference to your SnowGlobe plugin
 * @param id         Unique ID to use for the new sphere instance
 * @param tex        Texture for the new sphere instance
 */
Sphere::Sphere(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath)
    : Object(basePlugin, id, std::move(tex), filepath) {
    initShaders();

    // Init sphere vertex data.
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
    try {
        shaderProgram = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, basePlugin.getStringResource("shaders/sphere.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, basePlugin.getStringResource("shaders/sphere.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

Birds::Birds(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath)
    : Object(basePlugin, id, std::move(tex), filepath) {
    initShaders();

    // Load vertex data.
    loadObj(filepath, va);
}

void Birds::reloadShaders() {
    initShaders();
}

void Birds::initShaders() {
    try {
        shaderProgram = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, basePlugin.getStringResource("shaders/birds.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, basePlugin.getStringResource("shaders/birds.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

