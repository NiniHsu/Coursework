#include "SnowGlobe.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>

#include <imgui.h>

#include "Objects.h"
#include "core/core.h"

using namespace OGL4Core2::Plugins::PCVC::SnowGlobe;

static const double pi = std::acos(-1.0);

/**
 * @brief SnowGlobe constructor.
 * Initlizes all variables with meaningful values and initializes
 * geometry, objects and shaders used in the scene.
 */
SnowGlobe::SnowGlobe(const OGL4Core2::Core::Core& c)
    : Core::RenderPlugin(c),
    wWidth(10),
    wHeight(10),
    aspect(1.0f),
    lastMouseX(0.0),
    lastMouseY(0.0),
    moveMode(ObjectMoveMode::None),
    projMx(glm::mat4(1.0)),
    fbo(0),
    fboTexColor(0),
    fboTexId(0),
    fboTexNormals(0),
    fboTexPos(0),
    fboTexDepth(0),
    showFBOAtt(0),
    showparticleMode(0),
    maxParticles(1000),
    domeAlpha(0.1),
    useDayNightCycle(false),
    fovY(45.0),
    zNear(0.01f),
    zFar(50.0f),
    lightLong(0.0f),
    lightLat(180.0f),
    lightDist(4.0f),
    lightPos(glm::vec3(0.0f)),
    ambientColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    diffuseColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    specularColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    k_ambient(0.2f),
    k_diffuse(0.7f),
    k_specular(0.1f),
    k_exp(120.0f),
    texSkybox(0),
    lastshowparticleMode(0),
    lastmaxParticles(maxParticles),
    lastUsedParticle(0),
    drawParticleNum(0),
    vaCPU(0),
    vaGPU(0),
    vboCPU(0),
    positionBufferCPU(0),
    vboGPU(0),
    ssboGPU(0) {

    // Init Camera
    camera = std::make_shared<Core::OrbitCamera>(10.0f);
    core_.registerCamera(camera);

    // Initialize shaders, vertex arrays, skybox, and CPU particles
    initShaders();
    initVAs();
    initSkybox();
    initParticlesCPU();

    // Load textures from the "resources/textures" folder
    texBoard = getTextureResource("textures/snow.png"); 
    texSphere = getTextureResource("textures/eris.png");
    texPenguin = getTextureResource("textures/penguin.png");
    texDuck = getTextureResource("textures/duck.png");
    texBird = getTextureResource("textures/bird.png");
    texSnowflake = getTextureResource("textures/snowflake.png");

    // Setup the 3D scene in the dome
    std::shared_ptr<Object> o1 = std::make_shared<Base>(*this, 101, texBoard, "");
    o1->modelMx = glm::scale(o1->modelMx, glm::vec3(5.0f, 5.0f, 0.01f));
    objectList.emplace_back(o1);

    std::shared_ptr<Object> o2 = std::make_shared<Sphere>(*this, 102, texSphere, "");
    o2->modelMx = glm::translate(o2->modelMx, lightPos);
    o2->modelMx = glm::scale(o2->modelMx, glm::vec3(0.5f, 0.5f, 0.5f));
    objectList.emplace_back(o2);

    auto path3 = getResourceDirPath("models") / "penguin.obj";
    std::shared_ptr<Object> o3 = std::make_shared<Birds>(*this, 103, texPenguin, path3.string());
    o3->modelMx = glm::translate(o3->modelMx, glm::vec3(0.0f, -1.0f, 0.05f));
    o3->modelMx = glm::scale(o3->modelMx, glm::vec3(0.015f, 0.015f, 0.015f));
    objectList.emplace_back(o3);

    auto path4 = getResourceDirPath("models") / "duck.obj";
    std::shared_ptr<Object> o4 = std::make_shared<Birds>(*this, 104, texDuck, path4.string());
    o4->modelMx = glm::translate(o4->modelMx, glm::vec3(1.0f, 1.0f, 0.0f));
    o4->modelMx = glm::rotate(o4->modelMx, glm::radians(120.0f), glm::vec3(0.0, 0.0, 1.0));
    o4->modelMx = glm::scale(o4->modelMx, glm::vec3(0.015f, 0.015f, 0.015f));
    objectList.emplace_back(o4);

    auto path5 = getResourceDirPath("models") / "bird.obj";
    std::shared_ptr<Object> o5 = std::make_shared<Birds>(*this, 105, texBird, path5.string());
    o5->modelMx = glm::translate(o5->modelMx, glm::vec3(-1.0f, 0.8f, 0.0f));
    o5->modelMx = glm::rotate(o5->modelMx, glm::radians(340.0f), glm::vec3(0.0, 0.0, 1.0));
    o5->modelMx = glm::scale(o5->modelMx, glm::vec3(0.04f, 0.04f, 0.04f));
    objectList.emplace_back(o5);

    // Initialize clear color, enable depth testing and blend
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

/**
 * @brief SnowGlobe destructor.
 */
SnowGlobe::~SnowGlobe() {
    // --------------------------------------------------------------------------------
    //  Note: The glowl library uses the RAII principle. OpenGL objects are deleted in
    //        the destructor of the glowl wrapper objects. Therefore we must not delete
    //        them on our own. But keep this in mind and remember that this not always
    //        happens automatically.
    // --------------------------------------------------------------------------------
    glDeleteTextures(1, &fboTexColor);
    glDeleteTextures(1, &fboTexId);
    glDeleteTextures(1, &fboTexNormals); 
    glDeleteTextures(1, &fboTexPos);
    glDeleteTextures(1, &fboTexDepth);
    glDeleteTextures(1, &texSkybox);
    deleteFBOs();
    deleteParticlesCPU();
    deleteParticlesGPU();
    // Reset OpenGL state.
    glDisable(GL_DEPTH_TEST);
}

/**
 * @brief Render GUI.
 */
void SnowGlobe::renderGUI() {
    if (ImGui::CollapsingHeader("SnowGlobe", ImGuiTreeNodeFlags_DefaultOpen)) {
        camera->drawGUI();
        ImGui::Combo("FBO attach.", &showFBOAtt, "Color\0IDs\0Normals\0Position\0Depth\0Deferred\0"); 
        ImGui::Combo("Particle mode", &showparticleMode, "CPU\0GPU\0");
        ImGui::SliderFloat("domeAlpha", &domeAlpha, 0.0f, 1.0f);
        ImGui::InputInt("maxParticles", &maxParticles, 1000);
        maxParticles = std::clamp(maxParticles, 1000, 100000);
        ImGui::Checkbox("Day/Night Cycle", &useDayNightCycle);
        ImGui::SliderFloat("fovY", &fovY, 1.0f, 90.0f); 
        ImGui::SliderFloat("zNear", &zNear, 0.01f, zFar);
        ImGui::SliderFloat("zFar", &zFar, zNear, 100.0f);
        //ImGui::SliderFloat("lightLong", &lightLong, 0.0f, 360.0f);
        //ImGui::SliderFloat("lightLat", &lightLat, 0.0f, 360.0f);
        ImGui::ColorEdit3("Ambient", reinterpret_cast<float*>(&ambientColor), ImGuiColorEditFlags_Float);
        ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&diffuseColor), ImGuiColorEditFlags_Float);
        ImGui::ColorEdit3("Specular", reinterpret_cast<float*>(&specularColor), ImGuiColorEditFlags_Float);
        ImGui::SliderFloat("k_amb", &k_ambient, 0.0f, 1.0f);
        ImGui::SliderFloat("k_diff", &k_diffuse, 0.0f, 1.0f);
        ImGui::SliderFloat("k_spec", &k_specular, 0.0f, 1.0f);
        ImGui::SliderFloat("k_exp", &k_exp, 0.0f, 5000.0f);
    }
}

/**
 * @brief SnowGlobe render callback.
 */
void SnowGlobe::render() {
    renderGUI();
    updateLight();
    updateMatrices();

    if (showparticleMode == 0) { // Now: CPU particles mode
        if (lastshowparticleMode == 1) { // If mode has changed from GPU particles to CPU particles
            lastshowparticleMode = 0;
            deleteParticlesGPU();
            initParticlesCPU();
        }
        else if (lastmaxParticles != maxParticles) { // If the number of maximun particles has changed
            lastmaxParticles = maxParticles;
            deleteParticlesCPU();
            initParticlesCPU();
        }
        updateParticlesCPU();
    }
    else if (showparticleMode == 1) { // Now: GPU particles mode
        if (lastshowparticleMode == 0) { // If mode has changed from CPU particles to GPU particles
            lastshowparticleMode = 1;
            deleteParticlesCPU();
            initParticlesGPU();
        }
        else if (lastmaxParticles != maxParticles) { // If the number of maximun particles has changed
            lastmaxParticles = maxParticles;
            deleteParticlesGPU();
            initParticlesGPU();
        }
    }

    // First render pass to fill the FBOs
    drawToFBO();

    // Second render pass: a window filling quad is drawn and the FBO textures are used
    glViewport(0, 0, wWidth, wHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate camera position
    glm::vec4 cameraPos = glm::vec4(inverse(camera->viewMx()) * glm::vec4(0.0, 0.0, 0.0, 1.0));

    // Set up quadshader and uniform
    glm::mat4 orthoMx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
    shaderQuad->use();
    shaderQuad->setUniform("projMx", orthoMx);
    shaderQuad->setUniform("showFBOAtt", showFBOAtt);
    shaderQuad->setUniform("lightPos", lightPos);
    shaderDome->setUniform("cameraPos", glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z));
    shaderDome->setUniform("invViewProjMx", inverse(camera->viewMx()) * inverse(projMx));

    shaderQuad->setUniform("ambient", ambientColor);
    shaderQuad->setUniform("diffuse", diffuseColor);
    shaderQuad->setUniform("specular", specularColor);

    shaderQuad->setUniform("k_amb", k_ambient);
    shaderQuad->setUniform("k_diff", k_diffuse);
    shaderQuad->setUniform("k_spec", k_specular);
    shaderQuad->setUniform("k_exp", k_exp);

    // Bind color attachment and Set up texture uniform
    GLint unit = 0; // Color
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, fboTexColor);
    shaderQuad->setUniform("fboTexColor", unit);

    unit = 1; // ID
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, fboTexId);
    shaderQuad->setUniform("fboTexId", unit);

    unit = 2; // Normals
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, fboTexNormals);
    shaderQuad->setUniform("fboTexNormals", unit);

    unit = 3; // Positioin
    glActiveTexture(GL_TEXTURE0 + unit); 
    glBindTexture(GL_TEXTURE_2D, fboTexPos);
    shaderQuad->setUniform("fboTexPos", unit);

    unit = 4; // Depth
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, fboTexDepth);
    shaderQuad->setUniform("fboTexDepth", unit);

    // Draw
    vaQuad->draw();

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief SnowGlobe resize callback.
 * @param width  The current width of the window
 * @param height The current height of the window
 */
void SnowGlobe::resize(int width, int height) {
    wWidth = width;
    wHeight = height;
    aspect = static_cast<float>(wWidth) / static_cast<float>(wHeight);

    // Every time the window size changes, the size, of the fbo has to be adapted.
    deleteFBOs();
    initFBOs();
}

/**
 * @brief SnowGlobe keyboard callback.
 * @param key      Which key caused the event
 * @param action   Which key action was performed
 * @param mods     Which modifiers were active (e. g. shift)
 */
void SnowGlobe::keyboard(Core::Key key, Core::KeyAction action, [[maybe_unused]] Core::Mods mods) {
    if (action != Core::KeyAction::Press) {
        return;
    }

    if (key == Core::Key::R) {
        std::cout << "Reload shaders!" << std::endl;
        initShaders();
        for (const auto& object : objectList) {
            object->reloadShaders();
        }
    }
    else if (key == Core::Key::C) {
        showparticleMode = 0;
    }
    else if (key == Core::Key::G) {
        showparticleMode = 1;
    }
    else if (key >= Core::Key::Key1 && key <= Core::Key::Key6) {
        showFBOAtt = static_cast<int>(key) - static_cast<int>(Core::Key::Key1);
    }
}

/**
 * @brief SnowGlobe mouse callback.
 * @param button   Which mouse button caused the event
 * @param action   Which mouse action was performed
 * @param mods     Which modifiers were active (e. g. shift)
 */
void SnowGlobe::mouseButton(Core::MouseButton button, Core::MouseButtonAction action, Core::Mods mods) {

}

/**
 * @brief SnowGlobe mouse move callback.
 * Called after the mouse was moved, coordinates are measured in screen coordinates but
 * relative to the top-left corner of the window.
 * @param xpos     The x position of the mouse cursor
 * @param ypos     The y position of the mouse cursor
 */
void SnowGlobe::mouseMove(double xpos, double ypos) {
    lastMouseX = xpos;
    lastMouseY = ypos;
}

/**
 * @brief Init shaders for the window filling quad.
 */
void SnowGlobe::initShaders() {
    try {
        shaderQuad = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/quad.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/quad.frag")}});
    } catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    try {
        shaderSkybox = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/skybox.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/skybox.frag")} });
    } catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    try {
        shaderDome = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/dome.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/dome.frag")} });
    } catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
    
    try {
        shaderParticleCPU = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/particleCPU.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/particleCPU.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    try {
        shaderParticleCompute = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Compute, getStringResource("shaders/particleGPU.comp")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    try {
        shaderParticleGPU = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/particleGPU.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/particleGPU.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

/**
 * @brief Init vertex arrays.
 */
void SnowGlobe::initVAs() {
    // Create a vertex array for the window filling quad
    std::vector<float> quadVertices{
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
    std::vector<uint32_t> quadIndices{
        0, 1, 2,
        2, 1, 3
    };
    glowl::VertexLayout quadLayout{{0}, {{2, GL_FLOAT, GL_FALSE, 0}}};
    vaQuad = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{quadVertices}, quadIndices, quadLayout);

    // Create a vertex array for the skybox
    std::vector<float> skyboxVertices{
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
    };
    std::vector<uint32_t> skyboxIndices{
        0, 1, 2,    3, 4, 5,    6, 7, 8,    9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
    };
    glowl::VertexLayout skyboxLayout{ {0}, {{3, GL_FLOAT, GL_FALSE, 0}} };
    vaSkybox = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{skyboxVertices},
        skyboxIndices, skyboxLayout, GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLES);

    // Create a vertex array for the dome
    const int resTheta = 128;
    const int resPhi = 64;
    const double radiusSphere = 0.5;
    std::vector<float> domeVertices;
    std::vector<float> domeTexCoords;
    std::vector<GLuint> domeIndices;
    const float stackStep = pi / float(resPhi);
    const float sectorStep = 2 * pi / float(resTheta);
    for (int i = 0; i <= resPhi; i++) {
        float currentStack = pi / 2.0 - stackStep * float(i);
        for (int j = 0; j <= resTheta; j++) {
            float currentSector = j * sectorStep;
            // Vertex
            domeVertices.emplace_back(radiusSphere * cos(currentStack) * cos(currentSector)); // vertex.x
            domeVertices.emplace_back(radiusSphere * cos(currentStack) * sin(currentSector)); // vertex.y
            domeVertices.emplace_back(radiusSphere * sin(currentStack)); // vertex.z
            // Texture coordinate
            domeTexCoords.emplace_back(float(j) / resTheta); // textcoord.u
            domeTexCoords.emplace_back(float(i) / resPhi); // textcoord.v
        }
    }
    for (int i = 0; i < resPhi; i++) {
        GLuint currentVertexOffset = i * (resTheta + 1);
        GLuint nextVertexOffset = currentVertexOffset + resTheta + 1;
        for (int j = 0; j < resTheta; j++, currentVertexOffset++, nextVertexOffset++) {
            if (i != 0) {
                domeIndices.emplace_back(currentVertexOffset);
                domeIndices.emplace_back(nextVertexOffset);
                domeIndices.emplace_back(currentVertexOffset + 1);
            }
            if (i != (resPhi)) {
                domeIndices.emplace_back(currentVertexOffset + 1);
                domeIndices.emplace_back(nextVertexOffset);
                domeIndices.emplace_back(nextVertexOffset + 1);
            }
        }
    }
    glowl::VertexLayout domeLayout{
    {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {2, GL_FLOAT, GL_FALSE, 0}} };
    vaDome = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{domeVertices, domeTexCoords},
        domeIndices, domeLayout, GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLE_STRIP);
}

/**
 * @brief Initialize all frame buffer objects (FBOs).
 */
void SnowGlobe::initFBOs() {
    if (wWidth <= 0 || wHeight <= 0) {
        return;
    }

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create textures for three color attachments
    fboTexColor = createFBOTexture(wWidth, wHeight, GL_RGBA, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR); // Color
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexColor, 0);

    fboTexId = createFBOTexture(wWidth, wHeight, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR); // ID
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fboTexId, 0);

    fboTexNormals = createFBOTexture(wWidth, wHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR); // Normals
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fboTexNormals, 0);

    fboTexPos = createFBOTexture(wWidth, wHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR); // Position
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, fboTexPos, 0);

    // Create textures for depth attachment
    fboTexDepth = createFBOTexture(wWidth, wHeight, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fboTexDepth, 0);

    // Check whether the framebuffer is complpete
    checkFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * @brief Delete all framebuffer objects.
 */
void SnowGlobe::deleteFBOs() {
    glDeleteFramebuffers(1, &fbo);
}

/**
 * @brief Check status of bound framebuffer object (FBO).
 */
void SnowGlobe::checkFBOStatus() {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE: {
            std::cout << "FBO status: complete." << std::endl;
            break;
        }
        case GL_FRAMEBUFFER_UNDEFINED: {
            std::cerr << "FBO status: undefined." << std::endl;
            break;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
            std::cerr << "FBO status: incomplete attachment." << std::endl;
            break;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
            std::cerr << "FBO status: no buffers are attached to the FBO." << std::endl;
            break;
        }
        case GL_FRAMEBUFFER_UNSUPPORTED: {
            std::cerr << "FBO status: combination of internal buffer formats is not supported." << std::endl;
            break;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
            std::cerr << "FBO status: number of samples or the value for ... does not match." << std::endl;
            break;
        }
        default: {
            std::cerr << "FBO status: unknown framebuffer status error." << std::endl;
            break;
        }
    }
}

/**
 * @brief Create a texture for use in the framebuffer object.
 * @param width            Texture width
 * @param height           Texture height
 * @param internalFormat   Internal format of the texture
 * @param format           Format of the data: GL_RGB,...
 * @param type             Data type: GL_UNSIGNED_BYTE, GL_FLOAT,...
 * @param filter           Texture filter: GL_LINEAR or GL_NEAREST

 * @return texture handle
 */
GLuint SnowGlobe::createFBOTexture(int width, int height, const GLenum internalFormat, const GLenum format,
                                 const GLenum type, GLint filter) {
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

/**
 * @brief Update the projection matrices.
 */
void SnowGlobe::updateMatrices() {
    projMx = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
}

/**
 * @brief Update the light position.
 */
void SnowGlobe::updateLight() {
    if (useDayNightCycle) {
        lightLat += 0.2f;
    }
    lightPos.x = lightDist * cos(glm::radians(lightLat)) * cos(glm::radians(lightLong));
    lightPos.y = lightDist * cos(glm::radians(lightLat)) * sin(glm::radians(lightLong));
    lightPos.z = lightDist * sin(glm::radians(lightLat));

    // Update the model matrix of the sphere according to the light position
    glm::mat4 sphereModelMx = glm::mat4(1.0);
    sphereModelMx = glm::translate(sphereModelMx, lightPos);
    sphereModelMx = glm::scale(sphereModelMx, glm::vec3(0.5f, 0.5f, 0.5f));
    objectList[1]->modelMx = sphereModelMx;
}

/**
 * @brief Draw to framebuffer object.
 */
void SnowGlobe::drawToFBO() {
    if (!glIsFramebuffer(fbo)) {
        return;
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Set up draw buffers to three color attachments
    GLenum drawBuffer[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, drawBuffer);

    // Draw objects from objectList
    for (int i = 0; i < objectList.size(); i++)
        objectList[i]->draw(projMx, camera->viewMx());

    // Draw snow
    if (showparticleMode == 0) { // CPU particles mode
        shaderParticleCPU->use();
        shaderParticleCPU->setUniform("projMx", projMx);
        shaderParticleCPU->setUniform("viewMx", camera->viewMx());
        glActiveTexture(GL_TEXTURE0);
        texSnowflake->bindTexture();
        shaderParticleCPU->setUniform("tex", 0);

        glBindVertexArray(vaCPU);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboCPU);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glVertexAttribDivisor(0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferCPU);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glVertexAttribDivisor(1, 1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, drawParticleNum);
        glBindVertexArray(0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }
    else if (showparticleMode == 1) { // GPU particles mode
        updateParticlesGPU();

        shaderParticleGPU->use();
        shaderParticleGPU->setUniform("projMx", projMx);
        shaderParticleGPU->setUniform("viewMx", camera->viewMx());
        glActiveTexture(GL_TEXTURE0);
        texSnowflake->bindTexture();
        shaderParticleGPU->setUniform("tex", 0);

        glBindVertexArray(vaGPU);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboGPU);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glVertexAttribDivisor(0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboGPU);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboGPU);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, maxParticles);
        
        glDisableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    // Draw skybox
    glDepthFunc(GL_LEQUAL); //glDepthMask(GL_FALSE);
    shaderSkybox->use();
    shaderSkybox->setUniform("projMx", projMx);
    shaderSkybox->setUniform("viewMx", glm::mat4(glm::mat3(camera->viewMx())));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texSkybox);
    shaderSkybox->setUniform("tex", 0);
    vaSkybox->draw();
    glDepthFunc(GL_LESS); //glDepthMask(GL_TRUE);
    glUseProgram(0);

    // Draw dome
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shaderDome->use();
    glm::mat4 modelMx = glm::mat4(1.0f);
    modelMx = glm::translate(modelMx, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMx = glm::scale(modelMx, glm::vec3(5.0f, 5.0f, 5.0f));
    shaderDome->setUniform("modelMx", modelMx);
    shaderDome->setUniform("projMx", projMx);
    shaderDome->setUniform("viewMx", camera->viewMx());
    glm::mat3 normalMx = glm::mat3(transpose(inverse(modelMx)));
    shaderDome->setUniform("normalMx", normalMx);
    glm::vec4 cameraPos = glm::vec4(inverse(camera->viewMx()) * glm::vec4(0.0, 0.0, 0.0, 1.0));
    shaderDome->setUniform("cameraPos", glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z));
    shaderDome->setUniform("tex", 0);
    shaderDome->setUniform("domeAlpha", domeAlpha);
    vaDome->draw();
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void SnowGlobe::initSkybox() {
    std::vector<std::string> faces {
        getResourcePath("../resources/skybox/space_right.png").string(),    // +x
        getResourcePath("../resources/skybox/space_left.png").string(),     // -x
        getResourcePath("../resources/skybox/space_top.png").string(),      // +y
        getResourcePath("../resources/skybox/space_bottom.png").string(),   // -y  
        getResourcePath("../resources/skybox/space_front.png").string(),    // +z
        getResourcePath("../resources/skybox/space_back.png").string(),     // -z
    };

    glGenTextures(1, &texSkybox);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texSkybox);

    // Set up textures
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        auto image = getPngResource(faces[i], width, height);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void SnowGlobe::initParticlesCPU() {
    // Init particleContainer
    particleContainer.clear();
    for (int i = 0; i < maxParticles; i++) {
        Particle p;
        p.position = glm::vec3(0.0f);
        p.speed = 0.0f;
        p.life = 0.0f;
        particleContainer.push_back(p);
    }

    // Snow quad
    const std::vector<float> particleVertices = {
         0.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         1.0f, 1.0f, 0.0f,
    };

    // Set up va and buffers
    glGenVertexArrays(1, &vaCPU);
    glBindVertexArray(vaCPU);

    glGenBuffers(1, &vboCPU);
    glBindBuffer(GL_ARRAY_BUFFER, vboCPU);
    glBufferData(GL_ARRAY_BUFFER, particleVertices.size() * sizeof(float), &particleVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &positionBufferCPU);
    glBindBuffer(GL_ARRAY_BUFFER, positionBufferCPU);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void SnowGlobe::deleteParticlesCPU() {
    glDeleteVertexArrays(1, &vaCPU);
    glDeleteBuffers(1, &vboCPU);
    glDeleteBuffers(1, &positionBufferCPU);
}

void SnowGlobe::updateParticlesCPU() {
    int addNewParticle = maxParticles / 1000;
    float dt = 0.001f;
    particlePosition.clear();

    // Add new particles
    for (int i = 0; i < addNewParticle; i++) {
        int unusedParticle = firstUnusedParticle();
        respawnParticle(particleContainer[unusedParticle]);
    }

    // Update all particles
    for (int i = 0; i < maxParticles; i++) {
        Particle& p = particleContainer[i];
        p.life -= dt;
        if (p.life > 0.0f) {
            if (p.position.z > 0.01f) {
                p.position.x += (rand() % 2 - 1) * dt;
                p.position.y += (rand() % 2 - 1) * dt;
                p.position.z -= p.speed * dt;
            }
            if (distance(p.position, glm::vec3(0.0)) < 2.4f) {
                particlePosition.push_back(p.position.x);
                particlePosition.push_back(p.position.y);
                particlePosition.push_back(p.position.z);
                particlePosition.push_back(p.life);
            }
        }
    }
    drawParticleNum = particlePosition.size() / 4;
    if (drawParticleNum == 0) {
        for (int i = 0; i < 4; i++) particlePosition.push_back(0.0f);
    }

    // Update buffer data
    glBindVertexArray(vaCPU);
    glBindBuffer(GL_ARRAY_BUFFER, positionBufferCPU);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, drawParticleNum * sizeof(float) * 4, &particlePosition[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int SnowGlobe::firstUnusedParticle() {
    // Search from last used particle
    for (int i = lastUsedParticle; i < maxParticles; i++) {
        if (particleContainer[i].life <= 0.0f) {
            lastUsedParticle = i;
            return i;
        }
    }
    // Otherwise do linear search
    for (int i = 0; i < lastUsedParticle; i++) {
        if (particleContainer[i].life <= 0.0f) {
            lastUsedParticle = i;
            return i;
        }
    }
    // Override first particle if all others are alive
    lastUsedParticle = 0;
    return 0;
}

void SnowGlobe::respawnParticle(Particle& particle) {
    // Create new particle
    particle.position = glm::vec3(((rand() % 461) - 231) / 100.f, ((rand() % 461) - 231) / 100.f, 2.4f);
    particle.speed = rand() % 3 + 1;
    particle.life = 1.0f;
}

void SnowGlobe::initParticlesGPU() {
    lastUsedParticle = 0;

    // Init particleContainer
    particleContainerGPU.clear();
    for (int i = 0; i < maxParticles; i++) {
        glm::vec4 p = glm::vec4(0.0);
        particleContainerGPU.push_back(p);
    }

    // Snow quad
    const std::vector<float> particleVertices = {
         0.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         1.0f, 1.0f, 0.0f,
    };

    // Set up va and buffers
    glGenVertexArrays(1, &vaGPU);
    glBindVertexArray(vaGPU);

    glGenBuffers(1, &vboGPU);
    glBindBuffer(GL_ARRAY_BUFFER, vboGPU);
    glBufferData(GL_ARRAY_BUFFER, particleVertices.size() * sizeof(float), &particleVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ssboGPU);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboGPU);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticles * sizeof(glm::vec4), &particleContainerGPU[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindVertexArray(0);
}

void SnowGlobe::deleteParticlesGPU() {
    glDeleteVertexArrays(1, &vaGPU);
    glDeleteBuffers(1, &vboGPU);
    glDeleteBuffers(1, &ssboGPU);
}

void SnowGlobe::updateParticlesGPU() {
    int addNewParticle = maxParticles / 1000;
    if(lastUsedParticle <= maxParticles) lastUsedParticle += addNewParticle;

    shaderParticleCompute->use(); 
    shaderParticleCompute->setUniform("lastUsedParticle", lastUsedParticle);
    shaderParticleCompute->setUniform("seed", rand() % maxParticles);

    glBindVertexArray(vaGPU);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboGPU);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboGPU);

    // Implement computer shader
    glDispatchCompute(maxParticles / 100, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0); 
    glUseProgram(0);
}
