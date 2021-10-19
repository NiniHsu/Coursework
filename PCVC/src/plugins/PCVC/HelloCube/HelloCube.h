#ifndef OGL4CORE2_PLUGINS_PCVC_HELLOCUBE_HELLOCUBE_H
#define OGL4CORE2_PLUGINS_PCVC_HELLOCUBE_HELLOCUBE_H

#include <string>

#include <glad/gl.h>
#include <glm/matrix.hpp>

#include "core/pluginregister.h"
#include "core/renderplugin.h"

namespace OGL4Core2::Plugins::PCVC::HelloCube {

    class HelloCube : public Core::RenderPlugin {
        REGISTERPLUGIN(HelloCube, 101) // NOLINT

    public:
        static std::string name() { return "PCVC/HelloCube"; }

        explicit HelloCube(const Core::Core& c);
        ~HelloCube();

        void render() override;
        void resize(int width, int height) override;
        void mouseMove(double xpos, double ypos) override;

    private:
        void renderGUI();

        void initBuffers();
        void initShaderProgram();
        void initTexture();

        void resetCube();
        void resetCamera();

        // Matrix updates
        void setProjectionMatrix();
        void setViewMatrix();
        void setModelMatrix();

        // Window state
        float aspectRatio;

        // GL objects
        GLuint va;
        GLuint vbo;
        GLuint ibo;
        GLuint shaderProgram;
        int numCubeFaceTriangles;
        GLuint texture;

        // Matrices
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 modelMatrix;

        // Other variables
        // --------------------------------------------------------------------------------
        //  TODO: Define necessary variables.
        // --------------------------------------------------------------------------------

        // Shader
        const char* vertexShaderCode;
        const char* fragmentShaderCode;
        GLint vertexShader;
        GLint fragmentShader;

        // Mouse interaction
        double clickpopsX;
        double clickpopsY;
        float dollyVar;
        float fovVar;
        float translateVar;
        float rotateVar;
        float scaleVar;

        // Window size
        float windowW;
        float windowH;

        // GUI parameters
        glm::vec3 backgroundColor;
        int mouseControlMode;
        int transformCubeMode;
        float cameraDolly;
        float cameraFoVy;
        float cameraPitch;
        float cameraYaw;
        float objTranslateX;
        float objTranslateY;
        float objTranslateZ;
        float objRotateX;
        float objRotateY;
        float objRotateZ;
        float objScaleX;
        float objScaleY;
        float objScaleZ;
        bool showWireframe;
        bool showTexture;
        int patternFreq;
    };

} // namespace OGL4Core2::Plugins::PCVC::HelloCube

#endif // OGL4CORE2_PLUGINS_PCVC_HELLOCUBE_HELLOCUBE_H
