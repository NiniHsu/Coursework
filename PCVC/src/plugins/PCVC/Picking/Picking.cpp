#include "Picking.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>

#include <imgui.h>

#include "Objects.h"
#include "core/core.h"

using namespace OGL4Core2::Plugins::PCVC::Picking;

/**
 * @brief Picking constructor.
 * Initlizes all variables with meaningful values and initializes
 * geometry, objects and shaders used in the scene.
 */
Picking::Picking(const OGL4Core2::Core::Core& c)
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
      fboTexDepth(0),
      pickedObjNum(-1),
      lightZnear(0.1f),
      lightZfar(50.0f),
      lightProjMx(glm::mat4(1.0)),
      lightViewMx(glm::mat4(1.0)),
      lightFboWidth(2048),
      lightFboHeight(2048),
      lightFbo(0),
      lightFboTexColor(0),
      lightFboTexDepth(0),
      useWireframe(false),
      showFBOAtt(0),
      fovY(45.0),
      zNear(0.01f),
      zFar(20.0f),
      lightLong(0.0f),
      lightLat(90.00f),
      lightDist(10.0f),
      lightFoV(45.0f) {
    // Init Camera
    camera = std::make_shared<Core::OrbitCamera>(5.0f);
    core_.registerCamera(camera);

    // Initialize shaders and vertex arrays for quad and box.
    initShaders();
    initVAs();

    // --------------------------------------------------------------------------------
    //  TODO: Load textures from the "resources/textures" folder.
    //        Use the "getTextureResource" helper function.
    // --------------------------------------------------------------------------------
    texBoard = getTextureResource("textures/board.png");
    texDice = getTextureResource("textures/dice.png");
    texEarth = getTextureResource("textures/earth.png");

    // --------------------------------------------------------------------------------
    //  TODO: Setup the 3D scene. Add a dice, a sphere, and a torus.
    // --------------------------------------------------------------------------------
    // Add base object
    std::shared_ptr<Object> o1 = std::make_shared<Base>(*this, 100, texBoard);
    o1->modelMx = glm::translate(o1->modelMx, glm::vec3(0.0f, 0.0f, -0.6f));
    o1->modelMx = glm::scale(o1->modelMx, glm::vec3(5.0f, 5.0f, 0.01f));
    objectList.emplace_back(o1);

    // Add cube object
    std::shared_ptr<Object> o2 = std::make_shared<Cube>(*this, 111, texDice);
    o2->modelMx = glm::translate(o2->modelMx, glm::vec3(-0.5f, 1.3f, 0.0f));
    objectList.emplace_back(o2);

    // Add torus object
    std::shared_ptr<Object> o3 = std::make_shared<Torus>(*this, 122, nullptr);
    o3->modelMx = glm::translate(o3->modelMx, glm::vec3(-0.5f, -0.5f, -0.2f));
    o3->modelMx = glm::scale(o3->modelMx, glm::vec3(1.5f, 1.5f, 1.5f));
    objectList.emplace_back(o3);

    // Add sphere object
    std::shared_ptr<Object> o4 = std::make_shared<Sphere>(*this, 133, texEarth);
    o4->modelMx = glm::translate(o4->modelMx, glm::vec3(1.0f, 0.0f, 0.0f));
    o4->modelMx = glm::scale(o4->modelMx, glm::vec3(1.1f, 1.1f, 1.1f));
    objectList.emplace_back(o4);

    // Request some parameters
    GLint maxColAtt;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColAtt);
    std::cerr << "Maximum number of color attachments: " << maxColAtt << std::endl;

    GLint maxGeomOuputVerts;
    glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxGeomOuputVerts);
    std::cerr << "Maximum number of geometry output vertices: " << maxGeomOuputVerts << std::endl;

    // Initialize clear color and enable depth testing
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

/**
 * @brief Picking destructor.
 */
Picking::~Picking() {
    // --------------------------------------------------------------------------------
    //  Note: The glowl library uses the RAII principle. OpenGL objects are deleted in
    //        the destructor of the glowl wrapper objects. Therefore we must not delete
    //        them on our own. But keep this in mind and remember that this not always
    //        happens automatically.
    //
    //  TODO: Do not forget to clear all other allocated sources!
    // --------------------------------------------------------------------------------
    glDeleteTextures(1, &fboTexColor);
    glDeleteTextures(1, &fboTexId);
    glDeleteTextures(1, &fboTexNormals);
    glDeleteTextures(1, &fboTexDepth);
    deleteFBOs();
    // Reset OpenGL state.
    glDisable(GL_DEPTH_TEST);
}

/**
 * @brief Render GUI.
 */
void Picking::renderGUI() {
    if (ImGui::CollapsingHeader("Picking", ImGuiTreeNodeFlags_DefaultOpen)) {
        camera->drawGUI();
        ImGui::Checkbox("Wireframe", &useWireframe);
        // Dropdown menu to choose which framebuffer attachment to show
        ImGui::Combo("FBO attach.", &showFBOAtt, "Color\0IDs\0Normals\0Depth\0LightView\0LightDepth\0Deferred\0");

        // --------------------------------------------------------------------------------
        //  TODO: Setup ImGui elements for 'fovY', 'zNear', 'ZFar', 'lightLong', 'lightLat',
        //        'lightDist'. For the bonus task, you also need 'lightFoV'.
        // --------------------------------------------------------------------------------
        ImGui::SliderFloat("fovY", &fovY, 1.0f, 90.0f);
        ImGui::SliderFloat("zNear", &zNear, 0.01f, zFar);
        ImGui::SliderFloat("zFar", &zFar, zNear, 50.0f);
        ImGui::SliderFloat("lightLong", &lightLong, 0.0f, 360.0f);
        ImGui::SliderFloat("lightLat", &lightLat, 0.0f, 360.0f);
        ImGui::SliderFloat("lightDist", &lightDist, 1.0f, 50.0f);
        ImGui::SliderFloat("lightFoV", &lightFoV, 1.0f, 90.0f);
    }
}

/**
 * @brief Picking render callback.
 */
void Picking::render() {
    renderGUI();

    // Update the matrices for current frame.
    updateMatrices();

    // --------------------------------------------------------------------------------
    //  TODO: First render pass to fill the FBOs. Call the the drawTo... method(s).
    // --------------------------------------------------------------------------------
    drawToFBO();

    // --------------------------------------------------------------------------------
    //  TODO: In the second render pass, a window filling quad is drawn and the FBO
    //    textures are used for deferred shading.
    // --------------------------------------------------------------------------------
    glViewport(0, 0, wWidth, wHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate light position
    glm::vec3 lightPos(0.0);
    lightPos.x = lightDist * cos(glm::radians(lightLat)) * cos(glm::radians(lightLong));
    lightPos.y = lightDist * cos(glm::radians(lightLat)) * sin(glm::radians(lightLong));
    lightPos.z = lightDist * sin(glm::radians(lightLat));

    // Set up quadshader, uniform, and binding
    glm::mat4 orthoMx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
    shaderQuad->use();
    shaderQuad->setUniform("projMx", orthoMx);
    shaderQuad->setUniform("showFBOAtt", showFBOAtt);
    shaderQuad->setUniform("lightPos", lightPos);

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

    unit = 3; // Depth
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, fboTexDepth);
    shaderQuad->setUniform("fboTexDepth", unit);

    // Draw
    vaQuad->draw();

    glUseProgram(0); // release shaderQuad
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief Picking resize callback.
 * @param width  The current width of the window
 * @param height The current height of the window
 */
void Picking::resize(int width, int height) {
    wWidth = width;
    wHeight = height;
    aspect = static_cast<float>(wWidth) / static_cast<float>(wHeight);

    // Every time the window size changes, the size, of the fbo has to be adapted.
    deleteFBOs();
    initFBOs();
}

/**
 * @brief Picking keyboard callback.
 * @param key      Which key caused the event
 * @param action   Which key action was performed
 * @param mods     Which modifiers were active (e. g. shift)
 */
void Picking::keyboard(Core::Key key, Core::KeyAction action, [[maybe_unused]] Core::Mods mods) {
    if (action != Core::KeyAction::Press) {
        return;
    }

    if (key == Core::Key::R) {
        std::cout << "Reload shaders!" << std::endl;
        initShaders();
        for (const auto& object : objectList) {
            object->reloadShaders();
        }
    } else if (key >= Core::Key::Key1 && key <= Core::Key::Key7) {
        showFBOAtt = static_cast<int>(key) - static_cast<int>(Core::Key::Key1);
    }
}

/**
 * @brief Picking mouse callback.
 * @param button   Which mouse button caused the event
 * @param action   Which mouse action was performed
 * @param mods     Which modifiers were active (e. g. shift)
 */
void Picking::mouseButton(Core::MouseButton button, Core::MouseButtonAction action, Core::Mods mods) {
    // --------------------------------------------------------------------------------
    //  TODO: Add mouse button functionality.
    // --------------------------------------------------------------------------------
    if (action == Core::MouseButtonAction::Release) moveMode = ObjectMoveMode::None;
    else if ((action == Core::MouseButtonAction::Press) && mods.onlyShift()) {
        // Shift + Mouse left button: Pick object
        if (button == Core::MouseButton::Left) {
            glFlush();
            glFinish();

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glReadBuffer(GL_COLOR_ATTACHMENT1); // ID

            // Read the pixel color and identify which object does it belong to
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            unsigned char data[4];
            glReadPixels(lastMouseX, wHeight - lastMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
            int id = objectList[0]->colorToId(data);
            for (int i = 0; i < objectList.size(); i++) {
                if (id == objectList[i]->getId()) pickedObjNum = i;
            }
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        // Shift + Mouse middle button: Move object in xy-plane
        } else if ((pickedObjNum > 0) && (button == Core::MouseButton::Middle)) {
            moveMode = ObjectMoveMode::XY;
        // Shift + Mouse right button: Move object in z-plane
        } else if ((pickedObjNum > 0) && (button == Core::MouseButton::Right)) {
            moveMode = ObjectMoveMode::Z;
        }
    }
}

/**
 * @brief Picking mouse move callback.
 * Called after the mouse was moved, coordinates are measured in screen coordinates but
 * relative to the top-left corner of the window.
 * @param xpos     The x position of the mouse cursor
 * @param ypos     The y position of the mouse cursor
 */
void Picking::mouseMove(double xpos, double ypos) {
    // --------------------------------------------------------------------------------
    //  TODO: Add mouse move functionality.
    // --------------------------------------------------------------------------------
    float offsetX = xpos - lastMouseX;
    float offsetY = ypos - lastMouseY;
    float speedXY = 0.005;
    float speedZ = 0.01;

    // Calculate view matrixe after moving in xy-plane
    if (moveMode == ObjectMoveMode::XY) {
        glm::mat4 orthoMx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
        glm::mat4 objQuadMx = orthoMx * projMx * camera->viewMx() * objectList[pickedObjNum]->modelMx;
        objQuadMx = glm::translate(objQuadMx, glm::vec3(offsetX * speedXY, -offsetY * speedXY, 0.0f));
        objectList[pickedObjNum]->modelMx = inverse(camera->viewMx()) * inverse(projMx) * inverse(orthoMx) * objQuadMx;
    // Calculate view matrixe after moving in z-plane
    } else if (moveMode == ObjectMoveMode::Z) { 
        glm::mat4 orthoMx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
        glm::mat4 objQuadMx = orthoMx * projMx * camera->viewMx() * objectList[pickedObjNum]->modelMx;
        objQuadMx = glm::translate(objQuadMx, glm::vec3(0.0f, 0.0f, -offsetY * speedXY));
        objectList[pickedObjNum]->modelMx = inverse(camera->viewMx()) * inverse(projMx) * inverse(orthoMx) * objQuadMx;
    }
    // Update mouse position
    lastMouseX = xpos;
    lastMouseY = ypos;
}

/**
 * @brief Init shaders for the window filling quad and the box that is drawn around picked objects.
 */
void Picking::initShaders() {
    try {
        shaderQuad = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/quad.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/quad.frag")}});
    } catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    // --------------------------------------------------------------------------------
    //  TODO: Init box shader.
    // --------------------------------------------------------------------------------
    try {
        shaderBox = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/box.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/box.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

/**
 * @brief Init vertex arrays.
 */
void Picking::initVAs() {
    //  Create a vertex array for the window filling quad.
    std::vector<float> quadVertices{
        // clang-format off
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
        // clang-format on
    };
    std::vector<uint32_t> quadIndices{
        // clang-format off
        0, 1, 2,
        2, 1, 3
        // clang-format on
    };
    glowl::VertexLayout quadLayout{{0}, {{2, GL_FLOAT, GL_FALSE, 0}}};
    vaQuad = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{quadVertices}, quadIndices, quadLayout);

    // The box is used to indicate the selected object. It is made up of the eight corners of a unit cube that are
    // connected by lines.
    std::vector<float> boxVertices{
        // clang-format off
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        // clang-format on
    };

    std::vector<GLuint> boxEdges{
        // clang-format off
        0, 1,  0, 2,  1, 3,  2, 3,
        0, 4,  1, 5,  2, 6,  3, 7,
        4, 5,  4, 6,  5, 7,  6, 7
        // clang-format on
    };

    // --------------------------------------------------------------------------------
    //  TODO: Create the vertex arrays for box.
    // --------------------------------------------------------------------------------
    glowl::VertexLayout boxLayout{ {0}, {{3, GL_FLOAT, GL_FALSE, 0}} };
    vaBox = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{boxVertices}, boxEdges, boxLayout,
                                            GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_LINE_STRIP);
}

/**
 * @brief Initialize all frame buffer objects (FBOs).
 */
void Picking::initFBOs() {
    if (wWidth <= 0 || wHeight <= 0) {
        return;
    }

    // --------------------------------------------------------------------------------
    //  TODO: Create a frame buffer object (FBO) for multiple render targets.
    //        Use the createFBOTexture method to initialize an empty texture.
    // --------------------------------------------------------------------------------
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create textures for three color attachments
    fboTexColor = createFBOTexture(wWidth, wHeight, GL_RGBA, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR); // Color
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexColor, 0);

    fboTexId = createFBOTexture(wWidth, wHeight, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR); // ID
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fboTexId, 0);

    fboTexNormals = createFBOTexture(wWidth, wHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR); // Normals
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fboTexNormals, 0);

    // Create textures for depth attachment
    fboTexDepth = createFBOTexture(wWidth, wHeight, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fboTexDepth, 0);

    // Check whether the framebuffer is complpete
    checkFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --------------------------------------------------------------------------------
    //  TODO (BONUS TASK): Create a frame buffer object for the view from the spot light.
    // --------------------------------------------------------------------------------
}

/**
 * @brief Delete all framebuffer objects.
 */
void Picking::deleteFBOs() {
    // --------------------------------------------------------------------------------
    //  TODO: Clean up all FBOs.
    // --------------------------------------------------------------------------------
    glDeleteFramebuffers(1, &fbo);
}

/**
 * @brief Check status of bound framebuffer object (FBO).
 */
void Picking::checkFBOStatus() {
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
GLuint Picking::createFBOTexture(int width, int height, const GLenum internalFormat, const GLenum format,
                                 const GLenum type, GLint filter) {
    // --------------------------------------------------------------------------------
    //  TODO: Generate an empty 2D texture. Set min/mag filters. Set wrap mode in (s,t).
    // --------------------------------------------------------------------------------
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
 * @brief Update the matrices.
 */
void Picking::updateMatrices() {
    // --------------------------------------------------------------------------------
    //  TODO: Update the projection matrix (projMx).
    // --------------------------------------------------------------------------------
    projMx = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
    // --------------------------------------------------------------------------------
    //  TODO: Update the light matrices (for bonus task only).
    // --------------------------------------------------------------------------------
}

/**
 * @brief Draw to framebuffer object.
 */
void Picking::drawToFBO() {
    if (!glIsFramebuffer(fbo)) {
        return;
    }

    // --------------------------------------------------------------------------------
    //  TODO: Render the scene to the FBO.
    // --------------------------------------------------------------------------------
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up draw buffers to three color attachments
    GLenum drawBuffer[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, drawBuffer);

    // Draw objects from objectList
    for(int i = 0; i<objectList.size(); i++)
        objectList[i]->draw(projMx, camera->viewMx());

    // Draw box if picking
    if (pickedObjNum > 0) {
        shaderBox->use();
        shaderBox->setUniform("scale", 1.1f);
        shaderBox->setUniform("modelMx", objectList[pickedObjNum]->modelMx);
        shaderBox->setUniform("projMx", projMx);
        shaderBox->setUniform("viewMx", camera->viewMx());
        vaBox->draw();
        glUseProgram(0);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/**
 * @brief Draw view of spot light into corresponding FBO.
 */
void Picking::drawToLightFBO() {
    if (!glIsFramebuffer(lightFbo)) {
        return;
    }

    // --------------------------------------------------------------------------------
    //  TODO: Render the scene from the view of the light (bonus task only).
    // --------------------------------------------------------------------------------
}

