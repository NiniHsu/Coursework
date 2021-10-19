#include "SurfaceVis.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "core/core.h"

const int IDX_OFFSET = 10;

using namespace OGL4Core2::Plugins::PCVC::SurfaceVis;

/**
 * @brief SurfaceVis constructor.
 */
SurfaceVis::SurfaceVis(const OGL4Core2::Core::Core& c)
    : Core::RenderPlugin(c),
    wWidth(32),
    wHeight(32),
    lastMouseX(0.0),
    lastMouseY(0.0),
    zNear(0.01f),
    zFar(10.0f),
    pickedIdMove(0),
    projMx(glm::mat4(1.0f)),
    vaEmpty(0),
    ssbo(0),
    maxTessGenLevel(0),
    degree_p(3), // can assumed to be constant for this assignment
    degree_q(3), // can assumed to be constant for this assignment
    // --------------------------------------------------------------------------------
    //  TODO: Initialize self defined variables here.
    // --------------------------------------------------------------------------------
    fovY(45.0f),
    showBox(false),
    showNormals(false),
    useWireframe(false),
    showControlPoints(1),
    pointSize(10.0f),
    dataFilename("test.txt"),
    // --------------------------------------------------------------------------------
    //  TODO: Initialize self defined GUI variables here.
    // --------------------------------------------------------------------------------
    numControlPoints_n(4),
    numControlPoints_m(4),
    numControlPoints(numControlPoints_n* numControlPoints_m),
    pickedId(0),
    pickedPosition{ 0.0f, 0.0f, 0.0f },
    tessLevelInner(16),
    tessLevelOuter(16),
    ambientColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    diffuseColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    specularColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    k_ambient(0.2f),
    k_diffuse(0.7f),
    k_specular(0.0f),
    k_exp(120.0f),
    freq(4) {
    // Init Camera
    camera = std::make_shared<Core::OrbitCamera>(2.0f);
    core_.registerCamera(camera);

    // --------------------------------------------------------------------------------
    //  TODO: Check and save maximum allowed tessellation level to 'maxTessGenLevel'
    // --------------------------------------------------------------------------------
    glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessGenLevel);
    initShaders();
    initVAs();
    
    // --------------------------------------------------------------------------------
    //  TODO: Initialize a flat b-spline surface.
    // --------------------------------------------------------------------------------
    initControlPoints();
    initKnotVector();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); // Change depth function to overdraw on same depth for nicer control points.
}

/**
 * @brief SurfaceVis destructor.
 */
SurfaceVis::~SurfaceVis() {
    // --------------------------------------------------------------------------------
    //  TODO: Do not forget to clear all allocated resources.
    // --------------------------------------------------------------------------------
    glDeleteVertexArrays(1, &vaEmpty);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

/**
 * @brief GUI rendering.
 */
void SurfaceVis::renderGUI() {
    camera->drawGUI();

    ImGui::SliderFloat("FoVy", &fovY, 5.0f, 90.0f);
    ImGui::Checkbox("Show Box", &showBox);
    ImGui::Checkbox("Show Normals", &showNormals);
    ImGui::Checkbox("Wireframe", &useWireframe);
    ImGui::Combo("ShowCPoints", &showControlPoints, "no\0yes\0always\0");
    ImGui::SliderFloat("PointSize", &pointSize, 1.0f, 50.0f);
    ImGui::InputText("Filename", &dataFilename);
    if (ImGui::Button("Load File")) {
        loadControlPoints(dataFilename);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save File")) {
        saveControlPoints(dataFilename);
    }

    // --------------------------------------------------------------------------------
    //  TODO: Draw GUI for all added GUI variables.
    // --------------------------------------------------------------------------------
    ImGui::InputInt("numControlPoints_n", &numControlPoints_n);
    numControlPoints_n = std::clamp(numControlPoints_n, 2, 8);
    ImGui::InputInt("numControlPoints_m", &numControlPoints_m);
    numControlPoints_m = std::clamp(numControlPoints_m, 2, 8);
    ImGui::InputInt("pickedID", &pickedId);
    pickedId = std::clamp(pickedId, 0, numControlPoints);
    ImGui::DragFloat3("pickedPosition", pickedPosition, 0.01f, -5.0f, 5.0f);
    if (pickedId > 0) {
        if (pickedPosition[0] != controlPointsVertices[3 * (pickedId - 1)]) controlPointsVertices[3 * (pickedId - 1)] = pickedPosition[0];
        else if (pickedPosition[1] != controlPointsVertices[3 * (pickedId - 1) + 1]) controlPointsVertices[3 * (pickedId - 1) + 1] = pickedPosition[1];
        else if (pickedPosition[2] != controlPointsVertices[3 * (pickedId - 1) + 2]) controlPointsVertices[3 * (pickedId - 1) + 2] = pickedPosition[2];
        else { for (int i = 0; i < 3; i++) pickedPosition[i] = controlPointsVertices[(pickedId - 1) * 3 + i]; }
    }
    ImGui::InputInt("tessLevelInner", &tessLevelInner);
    tessLevelInner = std::clamp(tessLevelInner, 1, maxTessGenLevel);
    ImGui::InputInt("tessLevelOuter", &tessLevelOuter);
    tessLevelOuter = std::clamp(tessLevelOuter, 1, maxTessGenLevel);

    ImGui::ColorEdit3("Ambient", reinterpret_cast<float*>(&ambientColor), ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&diffuseColor), ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Specular", reinterpret_cast<float*>(&specularColor), ImGuiColorEditFlags_Float);

    ImGui::SliderFloat("k_amb", &k_ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("k_diff", &k_diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("k_spec", &k_specular, 0.0f, 1.0f);
    ImGui::SliderFloat("k_exp", &k_exp, 0.0f, 5000.0f);

    ImGui::InputInt("freq", &freq);
    freq = std::clamp(freq, 0, 100);
}

/**
 * @brief SurfaceVis render callback.
 */
void SurfaceVis::render() {
    renderGUI();

    // --------------------------------------------------------------------------------
    //  TODO: Draw to the fbo.
    // --------------------------------------------------------------------------------
    glViewport(0, 0, wWidth, wHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update projection matrix
    projMx = glm::perspective(glm::radians(fovY), static_cast<float>(wWidth) / static_cast<float>(wHeight), zNear, zFar);

    drawToFBO();

    glm::mat4 orthoMx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
    shaderQuad->use();
    shaderQuad->setUniform("projMx", orthoMx);

    // --------------------------------------------------------------------------------
    //  TODO: Draw the screen-filling quad using the previous fbo as input texture.
    // --------------------------------------------------------------------------------
    glActiveTexture(GL_TEXTURE0);
    fbo->bindColorbuffer(0);
    shaderQuad->setUniform("tex", 0);
    vaQuad->draw();

    // --------------------------------------------------------------------------------
    //  TODO: Clean up after rendering
    // --------------------------------------------------------------------------------
    glUseProgram(0);
}

/**
 * @brief SurfaceVis resize callback.
 * @param width
 * @param height
 */
void SurfaceVis::resize(int width, int height) {
    if (width > 0 && height > 0) {
        wWidth = width;
        wHeight = height;
        // --------------------------------------------------------------------------------
        //  TODO: Initilialize the FBO again with the new width and height
        // --------------------------------------------------------------------------------
        initFBO();
    }
}

/**
 * @brief SurfaceVis keyboard callback.
 * @param key      Which key caused the event
 * @param action   Which key action was performed
 * @param mods     Which modifiers were active (e. g. shift)
 */
void SurfaceVis::keyboard(Core::Key key, Core::KeyAction action, [[maybe_unused]] Core::Mods mods) {

    if (action != Core::KeyAction::Press) {
        return;
    }

    // --------------------------------------------------------------------------------
    //  TODO: Add keyboard functionality, use the core enums such as Core::Key::R:
    //    - Press 'R': Reload shaders.
    //    - Press 'X': Deselect control point.
    //    - Press 'Right': Select next control point.
    //    - Press 'Left': Select previous control point.
    //    - Press 'L': Load control points file.
    //    - Press 'S': Save control points file.
    //    - Press 'Backspace': Reset control points.
    // --------------------------------------------------------------------------------
    if (key == Core::Key::R) {
        initShaders();
    }
    else if (key == Core::Key::X) {
        pickedId = 0;
        for (int i = 0; i < 3; i++) pickedPosition[i] = 0;
    }
    else if (key == Core::Key::Right) {
        if (pickedId < numControlPoints) {
            pickedId++;
            for (int i = 0; i < 3; i++) pickedPosition[i] = controlPointsVertices[(pickedId - 1) * 3 + i];
        }
    }
    else if (key == Core::Key::Left) {
        if (pickedId > 1) {
            pickedId--;
            for (int i = 0; i < 3; i++) pickedPosition[i] = controlPointsVertices[(pickedId - 1) * 3 + i];
        }
    }
    else if (key == Core::Key::L) {
        loadControlPoints(dataFilename);
    }
    else if (key == Core::Key::S) {
        saveControlPoints(dataFilename);
    }
    else if (key == Core::Key::Space) {
        initControlPoints();
        initKnotVector();
    }
}

/**
 * @brief SurfaceVis mouse callback.
 * @param button   Which mouse button caused the event
 * @param action   Which mouse action was performed
 * @param mods     Which modifiers were active (e. g. shift)
 */
void SurfaceVis::mouseButton(Core::MouseButton button, Core::MouseButtonAction action, Core::Mods mods) {
    // --------------------------------------------------------------------------------
    //  TODO: Implement picking.
    // --------------------------------------------------------------------------------
    if ((action == Core::MouseButtonAction::Press) && mods.onlyShift() && (button == Core::MouseButton::Left)) {
        // Shift + Mouse left button: Pick
        fbo->bindToRead(1);

        // Read the pixel color and identify which object does it belong to
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        unsigned char data[3];
        glReadPixels(lastMouseX, wHeight - lastMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
        pickedId = colorToId(data);
        if (pickedId > 0) {
            for (int i = 0; i < 3; i++) pickedPosition[i] = controlPointsVertices[(pickedId - 1) * 3 + i];
        }
        else {
            for (int i = 0; i < 3; i++) pickedPosition[i] = 0.0f;
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
    else if ((pickedId > 0) && (action == Core::MouseButtonAction::Press) && mods.onlyShift() && (button == Core::MouseButton::Middle)) pickedIdMove = 1;
    else if ((pickedId > 0) && (action == Core::MouseButtonAction::Press) && mods.onlyShift() && (button == Core::MouseButton::Right)) pickedIdMove = 2;
    else pickedIdMove = 0;
}

/**
 * @brief SurfaceVis mouse move callback.
 * Called after the mouse was moved, coordinates are measured in screen coordinates but
 * relative to the top-left corner of the window.
 * @param xpos     The x position of the mouse cursor
 * @param ypos     The y position of the mouse cursor
 */
void SurfaceVis::mouseMove(double xpos, double ypos) {
    // --------------------------------------------------------------------------------
    //  TODO: Implement view corrected movement of the picked control point.
    //        - Shift + Right Mouse Button: move in xy-plane
    //        - Shift + Middle Mouse Button: move in z-direction
    //          (if you don't have a middle mouse button, you can also use another
    //           key in combination with shift)
    // --------------------------------------------------------------------------------
    if (pickedIdMove > 0) {
        // World coord. to screen coord.
        glm::vec4 worldPos = glm::vec4(pickedPosition[0], pickedPosition[1], pickedPosition[2], 1.0f);
        glm::vec4 clipPos = projMx * camera->viewMx() * worldPos;
        glm::vec4 ndcPos = clipPos / clipPos.w;
        glm::vec3 screenPos = glm::vec3((ndcPos.x + 1.0) * wWidth / 2.0f, -(ndcPos.y - 1.0f) * wHeight / 2.0f, ndcPos.z * (zFar - zNear) / 2.0f + (zFar + zNear) / 2.0f);

        // Move the selected control point in the xy-plane
        if (pickedIdMove == 1) {
            screenPos += glm::vec3(xpos - lastMouseX, ypos - lastMouseY, 0.0f);
        }
        // Move the selected control point in the z-plane
        else if (pickedIdMove == 2) {
            screenPos += glm::vec3(0.0f, 0.0f, (ypos - lastMouseY) * -0.0001f);
        }

        // Screen coord. to world coord.
        ndcPos = glm::vec4(2.0f * screenPos.x / wWidth - 1.0f, 1.0f - 2.0f * screenPos.y / wHeight, (2.0f * screenPos.z - (zFar + zNear)) / (zFar - zNear), 1.0f);
        clipPos = ndcPos;
        worldPos = inverse(projMx * camera->viewMx()) * clipPos;
        worldPos = worldPos / worldPos.w;

        // Update vertex array
        for (int i = 0; i < 3; i++) {
            pickedPosition[i] = worldPos[i];
            controlPointsVertices[3 * (pickedId - 1) + i] = pickedPosition[i];
        }
    }

    lastMouseX = xpos;
    lastMouseY = ypos;
}

/**
 * @brief Convert object id to unique color.
 * @param id       Object id
 * @return color
 */
glm::vec3 SurfaceVis::idToColor(unsigned int id) {
    glm::ivec3 color(0);
    color.r = id % 256;
    color.g = (id >> 8u) % 256;
    color.b = (id >> 16u) % 256;
    return glm::vec3(color) / 255.0f;
}

/**
 * @brief Convert color to object id.
 * @param buf     Color defined as 3-array [red, green, blue]
 * @return id
 */
unsigned int SurfaceVis::colorToId(const unsigned char col[3]) {
    auto b1 = static_cast<unsigned int>(col[0]);
    auto b2 = static_cast<unsigned int>(col[1]);
    auto b3 = static_cast<unsigned int>(col[2]);
    return (b3 << 16u) + (b2 << 8u) + b1;
}

/**
 * @brief Initialize shaders.
 */
void SurfaceVis::initShaders() {
    std::cout << "Load shader done" << std::endl;
    // Initialize shader for rendering fbo content
    try {
        shaderQuad = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/quad.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/quad.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    // Initialize shader for box rendering
    try {
        shaderBox = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/box.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/box.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    // Initialize shader for control point rendering
    try {
        shaderControlPoints = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/control-points.vert")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/control-points.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }

    // Initialize shader for b-spline surface
    // --------------------------------------------------------------------------------
    //  TODO: Implement shader creation for the B-Spline surface shader.
    // --------------------------------------------------------------------------------
    try {
        shaderBSplineSurface = std::make_unique<glowl::GLSLProgram>(glowl::GLSLProgram::ShaderSourceList{
            {glowl::GLSLProgram::ShaderType::Vertex, getStringResource("shaders/surface.vert")},
            {glowl::GLSLProgram::ShaderType::TessControl, getStringResource("shaders/surface.tesc")},
            {glowl::GLSLProgram::ShaderType::TessEvaluation, getStringResource("shaders/surface.tese")},
            {glowl::GLSLProgram::ShaderType::Fragment, getStringResource("shaders/surface.frag")} });
    }
    catch (glowl::GLSLProgramException& e) { std::cerr << e.what() << std::endl; }
}

/**
 * @brief Initialize VAs.
 */
void SurfaceVis::initVAs() {
    // Initialize VA for rendering fbo content
    const std::vector<float> quadVertices{
        // clang-format off
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        // clang-format on
    };

    const std::vector<GLuint> quadIndices{ 0, 1, 2, 3 };

    glowl::VertexLayout quadLayout{ {0}, {{2, GL_FLOAT, GL_FALSE, 0}} };
    vaQuad = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{quadVertices}, quadIndices, quadLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_TRIANGLE_STRIP);

    // Initialize VA for box rendering
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

    glowl::VertexLayout boxLayout{ {0}, {{3, GL_FLOAT, GL_FALSE, 0}} };
    vaBox = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{boxVertices}, boxEdges, boxLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_LINES);

    // Empty VA
    // --------------------------------------------------------------------------------
    //  TODO: Create an empty VA which will be used to draw our b-spline surface.
    //        You must bind the VA once to create it. 
    // --------------------------------------------------------------------------------
    glGenVertexArrays(1, &vaEmpty);
}

/**
 * @brief Initialize framebuffer object.
 */
void SurfaceVis::initFBO() {
    // --------------------------------------------------------------------------------
    //  TODO: Initialize the fbo (use default depth stencil type):
    //        - 1 color attachment for object colors
    //        - 1 color attachment for picking
    //        Don't forget to check the status of your fbo!
    // --------------------------------------------------------------------------------
    fbo = std::make_unique<glowl::FramebufferObject>(wWidth, wHeight, glowl::FramebufferObject::DEPTH24);
    fbo->bind();
    fbo->createColorAttachment(GL_RGBA32F, GL_RGBA, GL_UNSIGNED_BYTE); // Object color
    fbo->createColorAttachment(GL_RGB32F, GL_RGB, GL_UNSIGNED_BYTE); // Picking
    GLenum status = fbo->checkStatus(0);
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * @brief Draw to framebuffer object.
 */
void SurfaceVis::drawToFBO() {
    // --------------------------------------------------------------------------------
    //  TODO: Render to the fbo.
    //        - Draw the box if the variable 'showBox' is true
    //        - Draw the B-Spline surface using the empty vertex array 'emptyVA'
    //        - Draw the control net if 'showControlPoints' is greater than 0.
    //          Use 'pointSize' to set the size of drawn points.
    //          Don't forget to modify the depth test depending on the value of 'showControlPoints'.
    // --------------------------------------------------------------------------------
    fbo->bindToDraw();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the box
    if (showBox) {
        shaderBox->use();
        shaderBox->setUniform("projMx", projMx);
        shaderBox->setUniform("viewMx", camera->viewMx());
        vaBox->draw();
        glUseProgram(0);
    }

    // Update vertex array of control points if numControlPoints_n/numControlPoints_m changes
    if (numControlPoints != (numControlPoints_n * numControlPoints_m)) {
        numControlPoints = numControlPoints_n * numControlPoints_m;
        initControlPoints();
        if (pickedId > numControlPoints) {
            pickedId = numControlPoints;
            for (int i = 0; i < 3; i++) pickedPosition[i] = controlPointsVertices[(pickedId - 1) * 3 + i];
        }
        initKnotVector();
    }
    // Update vertex attributes of control points
    glowl::VertexLayout controlPointsLayout{ {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {3, GL_FLOAT, GL_FALSE, 0}} };
    vaControlPoints = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{controlPointsVertices, controlPointsColor}, controlPointsIndices, controlPointsLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_POINTS);
    vaControlPoints_LINES = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{controlPointsVertices, controlPointsColor}, controlPointsIndices, controlPointsLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_LINES);
    // Draw the control points
    if (showControlPoints > 0) {
        if (showControlPoints == 2) glDepthMask(GL_FALSE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glEnable(GL_LINE_SMOOTH);
        shaderControlPoints->use();
        shaderControlPoints->setUniform("projMx", projMx);
        shaderControlPoints->setUniform("viewMx", camera->viewMx());
        shaderControlPoints->setUniform("pickedIdCol", idToColor(pickedId));
        shaderControlPoints->setUniform("pointSize", pointSize);
        vaControlPoints->draw();
        vaControlPoints_LINES->draw();
        glUseProgram(0);
        glDepthMask(GL_TRUE);
    }

    // Check whether the Wireframe checkbox is checked
    if (useWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // Draw the B-Spline surface
    shaderBSplineSurface->use();
    glBindVertexArray(vaEmpty);
    shaderBSplineSurface->setUniform("tessLevelInner", tessLevelInner);
    shaderBSplineSurface->setUniform("tessLevelOuter", tessLevelOuter);
    shaderBSplineSurface->setUniform("projMx", projMx);
    shaderBSplineSurface->setUniform("viewMx", camera->viewMx());
    shaderBSplineSurface->setUniform("showNormals", showNormals);
    shaderBSplineSurface->setUniform("freq", freq);
    glPatchParameteri(GL_PATCH_VERTICES, 4); // Number of the vertices per patch
    glDrawArraysInstanced(GL_PATCHES, 0, 4, 1); // (primitives type, started index, #vertex * #patch, #draw)
    glBindVertexArray(0);
    glUseProgram(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/**
 * @brief Load control points from file.
 * @param filename
 */
void SurfaceVis::loadControlPoints(const std::string& filename) {
    auto path = getResourceDirPath("models") / filename;
    std::cout << "Load model: " << path.string() << std::endl;

    // --------------------------------------------------------------------------------
    //  TODO: Load the control points file from 'path' and initialize all related data.
    // --------------------------------------------------------------------------------
    std::ifstream file(path);
    file >> numControlPoints_n >> degree_p >> numControlPoints_m >> degree_q;

    // Load vertex
    float coord;
    controlPointsVertices.clear();
    while (file >> coord) controlPointsVertices.push_back(coord);
    file.close();

    // Init. index and idColor
    std::vector<float> controlPointsColor;
    std::vector<GLuint> controlPointsIndices;
    for (int i = 0; i < numControlPoints_n; i++) {
        for (int j = 0; j < numControlPoints_m; j++) {
            // Index
            int index = numControlPoints_m * i + j;
            if (j > 0) {
                controlPointsIndices.push_back(index - 1);
                controlPointsIndices.push_back(index);
            }
            if (i > 0) {
                controlPointsIndices.push_back(index - numControlPoints_m);
                controlPointsIndices.push_back(index);
            }

            // ID color
            glm::vec3 idColor = idToColor(index + 1);
            controlPointsColor.push_back(idColor.x);
            controlPointsColor.push_back(idColor.y);
            controlPointsColor.push_back(idColor.z);
        }
    }
    glowl::VertexLayout boxLayout{ {0}, {{3, GL_FLOAT, GL_FALSE, 0}, {3, GL_FLOAT, GL_FALSE, 0}} };
    vaControlPoints = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{controlPointsVertices, controlPointsColor}, controlPointsIndices, boxLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_POINTS);
    vaControlPoints_LINES = std::make_unique<glowl::Mesh>(std::vector<std::vector<float>>{controlPointsVertices, controlPointsColor}, controlPointsIndices, boxLayout,
        GL_UNSIGNED_INT, GL_STATIC_DRAW, GL_LINES);
}

/**
 * @brief Save control points to a file.
 * @param filename The name of the file
 */
void SurfaceVis::saveControlPoints(const std::string& filename) {
    auto path = getResourceDirPath("models") / filename;
    std::cout << "Save model: " << path.string() << std::endl;

    // --------------------------------------------------------------------------------
    //  TODO: Save the control points file to 'path'.
    // --------------------------------------------------------------------------------
    std::ofstream file(path);
    file << numControlPoints_n << " " << degree_p << std::endl;
    file << numControlPoints_m << " " << degree_q << std::endl;
    file << std::showpoint;
    for (float i = 0; i < numControlPoints; i++) {
        file << std::setw(10) << controlPointsVertices[3 * i] << " " << controlPointsVertices[3 * i + 1] << " "
            << controlPointsVertices[3 * i + 2] << std::endl;
    }
    file.close();
}

// --------------------------------------------------------------------------------
//  TODO: Implement all self defined methods, e. g. for:
//        - creating and clearing B-Spline related data
//        - updating the position of the picked control point
//        - etc ....
// --------------------------------------------------------------------------------

void SurfaceVis::initControlPoints() {
    // Clear the va, idColor, index
    controlPointsVertices.clear();
    controlPointsColor.clear();
    controlPointsIndices.clear();

    // Create control points
    float step_n = 1.0f / (numControlPoints_n - 1);
    float step_m = 1.0f / (numControlPoints_m - 1);
    for (int i = 0; i < numControlPoints_n; i++) {
        glm::vec3 startPoint = glm::vec3(-0.5f, 0.5f, 0.0f) - glm::vec3(0.0f, step_n * i, 0.0f);
        for (int j = 0; j < numControlPoints_m; j++) {
            // Vertex position
            controlPointsVertices.push_back(startPoint.x + step_m * j);
            controlPointsVertices.push_back(startPoint.y);
            controlPointsVertices.push_back(startPoint.z);

            // index
            int index = numControlPoints_m * i + j;
            if (j > 0) {
                controlPointsIndices.push_back(index - 1);
                controlPointsIndices.push_back(index);
            }
            if (i > 0) {
                controlPointsIndices.push_back(index - numControlPoints_m);
                controlPointsIndices.push_back(index);
            }

            // ID color
            glm::vec3 idColor = idToColor(index + 1);
            controlPointsColor.push_back(idColor.x);
            controlPointsColor.push_back(idColor.y);
            controlPointsColor.push_back(idColor.z);
        }
    }
}

void SurfaceVis::initKnotVector() {
    struct knotVector {
        std::vector<float> knotVector_u;
        std::vector<float> knotVector_v;
    } knotVector;

    // Construct the uniform knot vectors
    float step = 1.0f / float(numControlPoints_n - degree_p);
    for (int i = 0; i < numControlPoints_n + degree_p + 1; i++) {
        if (i < degree_p + 1) knotVector.knotVector_u.push_back(0.0f);
        else if (i > numControlPoints_n) knotVector.knotVector_u.push_back(1.0f);
        else knotVector.knotVector_u.push_back(knotVector.knotVector_u[i - 1] + step);
    }

    step = 1.0f / float(numControlPoints_m - degree_q);
    for (int i = 0; i < numControlPoints_m + degree_q + 1; i++) {
        if (i < degree_q + 1) knotVector.knotVector_v.push_back(0.0f);
        else if (i > numControlPoints_m) knotVector.knotVector_v.push_back(1.0f);
        else knotVector.knotVector_v.push_back(knotVector.knotVector_v[i - 1] + step);
    }

    // Set up ssbo
    /*glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(knotVector), &knotVector, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);*/

}
