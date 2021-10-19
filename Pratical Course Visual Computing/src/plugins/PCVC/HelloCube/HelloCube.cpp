#include "HelloCube.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "core/core.h"

using namespace OGL4Core2::Plugins::PCVC::HelloCube;
using namespace glm;

const static float INIT_CAMERA_DOLLY = 5.0f;
const static float INIT_CAMERA_FOVY = 45.0f;
const static float INIT_CAMERA_PITCH = -30.0f;
const static float INIT_CAMERA_YAW = -45.0f;

const static float MIN_CAMERA_PITCH = -89.99f;
const static float MAX_CAMERA_PITCH = 89.99f;

const static float INIT_WINDOW_W = 1280.0f;
const static float INIT_WINDOW_H = 800.0f;

const static float zFar = 100.0f;
const static float zNear = 0.1f;

/**
 * @brief HelloCube constructor
 */
HelloCube::HelloCube(const OGL4Core2::Core::Core& c)
    : Core::RenderPlugin(c),
    aspectRatio(1.0f),
    va(0),
    vbo(0),
    ibo(0),
    shaderProgram(0),
    numCubeFaceTriangles(0),
    texture(0),
    projectionMatrix(glm::mat4(1.0f)),
    viewMatrix(glm::mat4(1.0f)),
    modelMatrix(glm::mat4(1.0f)),
    // --------------------------------------------------------------------------------
    //  TODO: Initialize other variables!
    // --------------------------------------------------------------------------------
    vertexShader(0),
    fragmentShader(0),

    clickpopsX(-1),
    clickpopsY(-1),
    dollyVar(0.1f),
    fovVar(0.5f),
    translateVar(0.01f),
    rotateVar(0.5f),
    scaleVar(0.01f),

    windowW(INIT_WINDOW_W),
    windowH(INIT_WINDOW_H),

    backgroundColor(glm::vec3(0.2f, 0.2f, 0.2f)),
    mouseControlMode(0),
    transformCubeMode(0),
    cameraDolly(INIT_CAMERA_DOLLY),
    cameraFoVy(INIT_CAMERA_FOVY),
    cameraPitch(INIT_CAMERA_PITCH),
    cameraYaw(INIT_CAMERA_YAW),
    objTranslateX(0.0f),
    objTranslateY(0.0f),
    objTranslateZ(0.0f),
    objRotateX(0.0f),
    objRotateY(0.0f),
    objRotateZ(0.0f),
    objScaleX(1.0f),
    objScaleY(1.0f),
    objScaleZ(1.0f),
    showWireframe(false),
    showTexture(false),
    patternFreq(0) {
    // Init buffer, shader program and texture.
    initBuffers();
    initShaderProgram();
    initTexture();

    // --------------------------------------------------------------------------------
    //  TODO: Set other stuff if necessary!
    // --------------------------------------------------------------------------------

    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);
}

/**
 * @brief HelloCube destructor.
 */
HelloCube::~HelloCube() {
    // --------------------------------------------------------------------------------
    //  TODO: Do not forget to cleanup the plugin!
    // --------------------------------------------------------------------------------
    // Clean up buffers
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteVertexArrays(1, &va);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteTextures(1, &texture);

    // Clean up shader program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);

    // Reset OpenGL state.
    glDisable(GL_DEPTH_TEST);
}

/**
 * @brief Render GUI.
 */
void HelloCube::renderGUI() {
    bool cameraChanged = false;
    bool objectChanged = false;
    if (ImGui::CollapsingHeader("Hello Cube", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Background Color", reinterpret_cast<float*>(&backgroundColor), ImGuiColorEditFlags_Float);
        ImGui::Combo("Mouse Control", &mouseControlMode, "Camera\0Object\0");
        if (mouseControlMode == 0) {
            cameraChanged |= ImGui::SliderFloat("Camera Dolly", &cameraDolly, 0.1f, 100.0f);
            cameraChanged |= ImGui::SliderFloat("Camera FoVy", &cameraFoVy, 1.0f, 90.0f);
            cameraChanged |= ImGui::SliderFloat("Camera Pitch", &cameraPitch, MIN_CAMERA_PITCH, MAX_CAMERA_PITCH);
            cameraChanged |= ImGui::SliderFloat("Camera Yaw", &cameraYaw, -360.0f, 360.0f);
        } else {
            ImGui::Combo("TransformCube", &transformCubeMode, "Translate\0Rotate\0Scale\0");
            objectChanged |= ImGui::SliderFloat("Transl. X", &objTranslateX, -10.0f, 10.0f);
            objectChanged |= ImGui::SliderFloat("Transl. Y", &objTranslateY, -10.0f, 10.0f);
            objectChanged |= ImGui::SliderFloat("Transl. Z", &objTranslateZ, -10.0f, 10.0f);
            objectChanged |= ImGui::SliderFloat("Rotate X", &objRotateX, -360.0f, 360.0f);
            objectChanged |= ImGui::SliderFloat("Rotate Y", &objRotateY, -360.0f, 360.0f);
            objectChanged |= ImGui::SliderFloat("Rotate Z", &objRotateZ, -360.0f, 360.0f);
            objectChanged |= ImGui::SliderFloat("Scale X", &objScaleX, 0.1f, 10.0f);
            objectChanged |= ImGui::SliderFloat("Scale Y", &objScaleY, 0.1f, 10.0f);
            objectChanged |= ImGui::SliderFloat("Scale Z", &objScaleZ, 0.1f, 10.0f);
        }
        if (ImGui::Button("Reset cube")) {
            resetCube();
        }
        if (ImGui::Button("Reset camera")) {
            resetCamera();
        }
        if (ImGui::Button("Reset all")) {
            resetCube();
            resetCamera();
        }
        ImGui::Checkbox("Wireframe", &showWireframe);
        ImGui::Checkbox("Texture", &showTexture);
        ImGui::InputInt("Pattern Freq.", &patternFreq);
        patternFreq = std::max(0, std::min(15, patternFreq));
        if (cameraChanged) {
            // --------------------------------------------------------------------------------
            //  TODO: Handle camera parameter changes!
            // --------------------------------------------------------------------------------

        }
        if (objectChanged) {
            // --------------------------------------------------------------------------------
            //  TODO: Handle object parameter changes!
            // --------------------------------------------------------------------------------
        }
    }
}

/**
 * @brief HelloCube render callback.
 */
void HelloCube::render() {
    renderGUI();

    // --------------------------------------------------------------------------------
    //  TODO: Implement rendering!
    // --------------------------------------------------------------------------------
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear buffers
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1.0f); // Set up backgroundcolor

    // Check whether the Wireframe checkbox is checked
    if (showWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(shaderProgram);

    // Pass patternFreq to cube.frag
    GLint patternFreqUniformLocation = glGetUniformLocation(shaderProgram, "patternFreq");
    glUniform1i(patternFreqUniformLocation, patternFreq);

    // Set up texture
    int showTextureInt = 0;
    if (showTexture) showTextureInt = 1;
    GLint textureUniformLocation = glGetUniformLocation(shaderProgram, "showTextureInt");
    glUniform1i(textureUniformLocation, showTextureInt);

    // Create transform matrix
    setModelMatrix();
    setViewMatrix();
    setProjectionMatrix();

    // Draw the object
    glBindVertexArray(va);
    glDrawElements(GL_TRIANGLES, 3 * numCubeFaceTriangles, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
}

/**
 * @brief HelloCube resize callback.
 * @param width
 * @param height
 */
void HelloCube::resize(int width, int height) {
    // --------------------------------------------------------------------------------
    //  TODO: Handle resize event!
    // --------------------------------------------------------------------------------
    // Set the viewport with new window size
    glViewport(0, 0, width, height);
    windowW = width;
    windowH = height;
}

/**
 * @brief HellCube mouse move callback.
 * @param xpos
 * @param ypos
 */
void HelloCube::mouseMove(double xpos, double ypos) {
    // --------------------------------------------------------------------------------
    //  TODO: Handle camera and object control!
    // --------------------------------------------------------------------------------
    // When the mouse left-click occurs
    if (this->core_.isMouseButtonPressed(Core::MouseButton::Left)) {
        // If the mouse drags, calculate the movement of the mouseand modify the corresponding value
        if (clickpopsX != -1) {
            // Calculate the direction of the mouse movement is positive or negative
            float movementX;
            float movementY;
            if (xpos - clickpopsX > 0.0f) movementX = 1.0f;
            else if (xpos - clickpopsX < 0.0f) movementX = -1.0f;
            else movementX = 0.0f;
            if (ypos - clickpopsY > 0.0f) movementY = 1.0f;
            else if (ypos - clickpopsY < 0.0f) movementY = -1.0f;
            else movementY = 0.0f;

            // Control the camera
            if (mouseControlMode == 0) {
                cameraYaw = std::min(std::max(cameraYaw - movementX * rotateVar, -360.0f), 360.0f);
                cameraPitch = std::min(std::max(cameraPitch - movementY * rotateVar, MIN_CAMERA_PITCH), MAX_CAMERA_PITCH);
                // Control the object
            } else {
                if (transformCubeMode == 0) { // Translate
                    objTranslateX = std::min(std::max(objTranslateX + movementX * translateVar, -10.f), 10.0f);
                    objTranslateY = std::min(std::max(objTranslateY + movementY * translateVar, -10.f), 10.0f);
                } else if (transformCubeMode == 1) { // Rotate
                    objRotateY = std::min(std::max(objRotateY + movementX * rotateVar, -360.f), 360.f);
                    objRotateX = std::min(std::max(objRotateX + movementY * rotateVar, -360.f), 360.f);
                } else if (transformCubeMode == 2) { // Scale
                    objScaleX = std::min(std::max(objScaleX + movementX * scaleVar, 0.1f), 10.0f);
                    objScaleY = std::min(std::max(objScaleY + movementY * scaleVar, 0.1f), 10.0f);
                }
            }
            // If the mouse clicks, store the clicking position
        } else {
            clickpopsX = xpos;
            clickpopsY = ypos;
        }
        // When mouse middle-click occurs
    } else if (this->core_.isMouseButtonPressed(Core::MouseButton::Middle)) {
        // If the mouse drags, calculate the movement of the mouseand modify the corresponding value
        if (clickpopsX != -1) {
            // Calculate the direction of the mouse movement is positive or negative
            float movementX;
            float movementY;
            if (xpos - clickpopsX > 0.0f) movementX = 1.0f;
            else if (xpos - clickpopsX < 0.0f) movementX = -1.0f;
            else movementX = 0.0f;
            if (ypos - clickpopsY > 0.0f) movementY = 1.0f;
            else if (ypos - clickpopsY < 0.0f) movementY = -1.0f;
            else movementY = 0.0f;

            // Control the camera
            if (mouseControlMode == 0) {
                cameraFoVy = std::min(std::max(cameraFoVy - movementY * fovVar, 1.0f), 90.0f);
                // Control the object
            } else {
                // Translate
                if (transformCubeMode == 0) {
                    objTranslateX = std::min(std::max(objTranslateX + movementX * translateVar, -10.f), 10.0f);
                    objTranslateZ = std::min(std::max(objTranslateZ + movementY * translateVar, -10.f), 10.0f);
                    // Rotate
                } else if (transformCubeMode == 1) {
                    objRotateX = std::min(std::max(objRotateX + movementX * rotateVar, -360.f), 360.f);
                    objRotateZ = std::min(std::max(objRotateZ + movementY * rotateVar, -360.f), 360.f);
                    // Scale
                } else if (transformCubeMode == 2) {
                    objScaleX = std::min(std::max(objScaleX + movementX * scaleVar, 0.1f), 10.0f);
                    objScaleZ = std::min(std::max(objScaleZ + movementY * scaleVar, 0.1f), 10.0f);
                }
            }
            // If the mouse clicks, store the clicking position
        } else {
            clickpopsX = xpos;
            clickpopsY = ypos;
        }
        // When mouse right-click occurs
    } else if (this->core_.isMouseButtonPressed(Core::MouseButton::Right)) {
        // If the mouse drags, calculate the movement of the mouseand modify the corresponding value
        if (clickpopsX != -1) {
            // Calculate the direction of the mouse movement is positive or negative
            float movementX;
            float movementY;
            if (xpos - clickpopsX > 0.0f) movementX = 1.0f;
            else if (xpos - clickpopsX < 0.0f) movementX = -1.0f;
            else movementX = 0.0f;
            if (ypos - clickpopsY > 0.0f) movementY = 1.0f;
            else if (ypos - clickpopsY < 0.0f) movementY = -1.0f;
            else movementY = 0.0f;

            // Control the camera
            if (mouseControlMode == 0) {
                cameraDolly = std::min(std::max(cameraDolly - movementY * dollyVar, 0.1f), 100.0f);
                // Control the object
            } else {
                // Translate
                if (transformCubeMode == 0) {
                    objTranslateY = std::min(std::max(objTranslateY + movementX * translateVar, -10.f), 10.0f);
                    objTranslateZ = std::min(std::max(objTranslateZ + movementY * translateVar, -10.f), 10.0f);
                    // Rotate
                } else if (transformCubeMode == 1) {
                    objRotateY = std::min(std::max(objRotateY + movementX * rotateVar, -360.f), 360.f);
                    objRotateZ = std::min(std::max(objRotateZ + movementY * rotateVar, -360.f), 360.f);
                    // Scale
                } else if (transformCubeMode == 2) {
                    objScaleY = std::min(std::max(objScaleY + movementX * scaleVar, 0.1f), 10.0f);
                    objScaleZ = std::min(std::max(objScaleZ + movementY * scaleVar, 0.1f), 10.0f);
                }
            }
            // If the mouse clicks, store the clicking position
        } else {
            clickpopsX = xpos;
            clickpopsY = ypos;
        }
        // No click occurs
    } else {
        clickpopsX = -1;
        clickpopsY = -1;
    }
}

/**
 * @brief Initialize buffers.
 */
void HelloCube::initBuffers() {
    // --------------------------------------------------------------------------------
    //  TODO: Init vertex buffer and index buffer!
    // --------------------------------------------------------------------------------
    const int numCubeVertices = 14;

    const std::vector<float> cubeVertices{          // [index] : [vertex] : [face]
        -0.5f, -0.5f, -0.5f, 1.0f,   0.0f,  0.5f,   // 0  : left  bottom back : left
        -0.5f, -0.5f, -0.5f, 1.0f,  0.25f, 0.25f,   // 1  : left  bottom back : bottom
        -0.5f, -0.5f, -0.5f, 1.0f,   1.0f,  0.5f,   // 2  : left  bottom back : back
         0.5f, -0.5f, -0.5f, 1.0f,  0.75f,  0.5f,   // 3  : right bottom back : right, back
         0.5f, -0.5f, -0.5f, 1.0f,   0.5f, 0.25f,   // 4  : right bottom back : bottom
        -0.5f,  0.5f, -0.5f, 1.0f,   0.0f, 0.75f,   // 5  : left  top    back : left
        -0.5f,  0.5f, -0.5f, 1.0f,  0.25f,  1.0f,   // 6  : left  top    back : top
        -0.5f,  0.5f, -0.5f, 1.0f,   1.0f, 0.75f,   // 7  : left  top    back : back
         0.5f,  0.5f, -0.5f, 1.0f,  0.75f, 0.75f,   // 8  : right top    back : right, back
         0.5f,  0.5f, -0.5f, 1.0f,   0.5f,  1.0f,   // 9  : right top    back : top
        -0.5f, -0.5f,  0.5f, 1.0f,  0.25f,  0.5f,   // 10 : left  bottom front : left, bottom, front
         0.5f, -0.5f,  0.5f, 1.0f,   0.5f,  0.5f,   // 11 : right bottom front : right, bottom, front
        -0.5f,  0.5f,  0.5f, 1.0f,  0.25f, 0.75f,   // 12 : left  top    front : left, top, front
         0.5f,  0.5f,  0.5f, 1.0f,   0.5f, 0.75f    // 13 : right top    front : right, top, front
    };

    numCubeFaceTriangles = 12;

    const std::vector<GLuint> cubeFaces{
         2,  7,  3,   3,  7,  8, // back
        11, 13, 10,  10, 13, 12, // front
         4, 11,  1,   1, 11, 10, // bottom
        13,  9, 12,  12,  9,  6, // top
        10, 12,  0,   0, 12,  5, // left
         3,  8, 11,  11,  8, 13 // right
    };

    // Set up VAO
    glGenVertexArrays(1, &va);
    glBindVertexArray(va);

    // Set up VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(float), &cubeVertices[0], GL_STATIC_DRAW);

    // Set up EBO
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeFaces.size() * sizeof(int), &cubeFaces[0], GL_STATIC_DRAW);

    // define position attribute
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // define texture coord. attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
}


/**
 * @brief HelloCube init shaders
 */
void HelloCube::initShaderProgram() {
    // --------------------------------------------------------------------------------
    //  TODO: Init shader program!
    // --------------------------------------------------------------------------------

    // Read glsl file
    std::string vertexShaderString = getStringResource(getResourcePath("../resources/cube.vert").string());
    std::string fragmentShaderString = getStringResource(getResourcePath("../resources/cube.frag").string());
    vertexShaderCode = vertexShaderString.c_str();
    fragmentShaderCode = fragmentShaderString.c_str();

    // Set up vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);

    // Check the compilation of the vertex shader
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::VERTEXSHADER::COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // Set up fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);

    // Check the compilation of the fragment shader
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::FRAGMENTSHADER::COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // Set up shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check the link of the shader program
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADERPROGRAM::LINK FAILED\n" << infoLog << std::endl;
    }
}

/**
 * @brief Initialize textures.
 */
void HelloCube::initTexture() {
    // --------------------------------------------------------------------------------
    //  TODO: Init texture!
    // --------------------------------------------------------------------------------
    int width, height;
    auto image = getPngResource(getResourcePath("../resources/texture.png").string(), width, height);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/**
 * @brief Reset cube.
 */
void HelloCube::resetCube() {
    // --------------------------------------------------------------------------------
    //  TODO: Reset cube!
    // --------------------------------------------------------------------------------
    objTranslateX = 0.0f;
    objTranslateY = 0.0f;
    objTranslateZ = 0.0f;
    objRotateX = 0.0f;
    objRotateY = 0.0f;
    objRotateZ = 0.0f;
    objScaleX = 1.0f;
    objScaleY = 1.0f;
    objScaleZ = 1.0f;
}

/**
 * @brief Reset camera.
 */
void HelloCube::resetCamera() {
    // --------------------------------------------------------------------------------
    //  TODO: Reset camera!
    // --------------------------------------------------------------------------------
    cameraDolly = INIT_CAMERA_DOLLY;
    cameraFoVy = INIT_CAMERA_FOVY;
    cameraPitch = INIT_CAMERA_PITCH;
    cameraYaw = INIT_CAMERA_YAW;
}

/**
 * @brief Set projection matrix.
 */
void HelloCube::setProjectionMatrix() {
    // --------------------------------------------------------------------------------
    //  TODO: Set projection matrix!
    // --------------------------------------------------------------------------------
    float tanf = tan(radians(cameraFoVy / 2));
    float aspect = windowW / windowH;
    projectionMatrix = {
        1.0f / (aspect * tanf), 0.0f,       0.0f,                               0.0f,
        0.0f,                   1.0 / tanf, 0.0f,                               0.0f,
        0.0f,                   0.0f,       -(zFar + zNear) / (zFar - zNear),   -1.0,
        0.0f,                   0.0f,       -2 * zFar * zNear / (zFar - zNear), 0.0f
    };

    // Pass projection matrix to cube.vert
    GLint projectionMatrixUniformLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

/**
 * @brief Set view matrix.
 */
void HelloCube::setViewMatrix() {
    // --------------------------------------------------------------------------------
    //  TODO: Set view matrix!
    // --------------------------------------------------------------------------------
    // Rotation matrix for x-asis
    mat4 viewrotateXMatrix = {
        1.0f, 0.0f,                      0.0f,                       0.0f,
        0.0f, cos(radians(cameraPitch)), -sin(radians(cameraPitch)), 0.0f,
        0.0f, sin(radians(cameraPitch)), cos(radians(cameraPitch)),  0.0f,
        0.0f, 0.0f,                      0.0f,                       1.0f
    };

    // Rotation matrix for y-asis
    mat4 viewrotateYMatrix = {
        cos(radians(cameraYaw)),  0.0f, sin(radians(cameraYaw)), 0.0f,
        0.0f,                     1.0f, 0.0f,                    0.0f,
        -sin(radians(cameraYaw)), 0.0f, cos(radians(cameraYaw)), 0.0f,
        0.0f,                     0.0f, 0.0f,                    1.0f
    };

    // Translation matrix
    mat4 viewtranslateMatrix = {
        1.0f, 0.0f, 0.0f,  0.0f,
        0.0f, 1.0f, 0.0f,  0.0f,
        0.0f, 0.0f, 1.0f,  0.0f,
        0.0f, 0.0f, -cameraDolly, 1.0f
    };

    viewMatrix = viewtranslateMatrix * viewrotateXMatrix * viewrotateYMatrix * mat4(1.0);

    // Pass view matrix to cube.vert
    GLint viewMatrixUniformLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    glUniformMatrix4fv(viewMatrixUniformLocation, 1, GL_FALSE, &viewMatrix[0][0]);
}

/**
 * @brief Set model matrix.
 */
void HelloCube::setModelMatrix() {
    // --------------------------------------------------------------------------------
    //  TODO: Set model matrix!
    // --------------------------------------------------------------------------------

    // Translation matrix
    mat4 translateMatrix = {
        1.0f,          0.0f,          0.0f,          0.0f,
        0.0f,          1.0f,          0.0f,          0.0f,
        0.0f,          0.0f,          1.0f,          0.0f,
        objTranslateX, objTranslateY, objTranslateZ, 1.0f
    };

    // Rotation matrix for x-asis
    mat4 rotateXMatrix = {
        1.0f, 0.0f,                     0.0f,                      0.0f,
        0.0f, cos(radians(objRotateX)), -sin(radians(objRotateX)), 0.0f,
        0.0f, sin(radians(objRotateX)), cos(radians(objRotateX)),  0.0f,
        0.0f, 0.0f,                     0.0f,                      1.0f
    };

    // Rotation matrix for y-asis
    mat4 rotateYMatrix = {
        cos(radians(objRotateY)),  0.0f, sin(radians(objRotateY)), 0.0f,
        0.0f,                      1.0f, 0.0f,                     0.0f,
        -sin(radians(objRotateY)), 0.0f, cos(radians(objRotateY)), 0.0f,
        0.0f,                      0.0f, 0.0f,                     1.0f
    };

    // Rotation matrix for z-asis
    mat4 rotateZMatrix = {
        cos(radians(objRotateZ)), -sin(radians(objRotateZ)), 0.0f, 0.0f,
        sin(radians(objRotateZ)), cos(radians(objRotateZ)),  0.0f, 0.0f,
        0.0f,                     0.0f,                      1.0f, 0.0f,
        0.0f,                     0.0f,                      0.0f, 1.0f
    };
    mat4 rotateMatrix = rotateXMatrix * rotateYMatrix * rotateZMatrix;

    // Scaling matrix
    mat4 scaleMatrix = {
        objScaleX,   0.0f,       0.0f,       0.0f,
        0.0f,        objScaleY,  0.0f,       0.0f,
        0.0f,        0.0f,       objScaleZ,  0.0f,
        0.0f,        0.0f,       0.0f,       1.0f
    };

    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;

    // Pass model matrix to cube.vert
    GLint modelMatrixUniformLocation = glGetUniformLocation(shaderProgram, "modelMatrix");
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, &modelMatrix[0][0]);
}
