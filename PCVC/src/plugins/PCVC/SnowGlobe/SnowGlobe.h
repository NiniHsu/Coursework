#ifndef OGL4CORE2_PLUGINS_PCVC_SNOWGLOBE_SNOWGLOBE_H
#define OGL4CORE2_PLUGINS_PCVC_SNOWGLOBE_SNOWGLOBE_H

#include <memory>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glowl/glowl.h>

#include "core/camera/orbitcamera.h"
#include "core/pluginregister.h"
#include "core/renderplugin.h"

namespace OGL4Core2::Plugins::PCVC::SnowGlobe {
    struct Particle {
        glm::vec3 position;
        float life;
        float speed;
    };

    class Object;

    class SnowGlobe : public Core::RenderPlugin {
        REGISTERPLUGIN(SnowGlobe, 105) // NOLINT

    public:
        static std::string name() { return "PCVC/SnowGlobe"; }

        explicit SnowGlobe(const Core::Core& c);
        ~SnowGlobe();

        void render() override;
        void resize(int width, int height) override;
        void keyboard(Core::Key key, Core::KeyAction action, Core::Mods mods) override;
        void mouseButton(Core::MouseButton button, Core::MouseButtonAction action, Core::Mods mods) override;
        void mouseMove(double xpos, double ypos) override;

    private:
        enum class ObjectMoveMode {
            None = 0,
            XY,
            Z,
        };

        void renderGUI();

        void initShaders();

        void initVAs();

        void initFBOs();
        void deleteFBOs();
        void checkFBOStatus();
        GLuint createFBOTexture(int width, int height, GLenum internalFormat, GLenum format, GLenum type, GLint filter);

        void updateMatrices();

        void updateLight();

        void drawToFBO();

        void initSkybox();

        // CPU particles
        void initParticlesCPU();
        void deleteParticlesCPU();
        void updateParticlesCPU();
        int firstUnusedParticle();
        void respawnParticle(Particle &particle);

        // GPU particles
        void initParticlesGPU();
        void deleteParticlesGPU();
        void updateParticlesGPU();

        // Window state
        int wWidth;              //!< width of the window
        int wHeight;             //!< height of the window
        float aspect;            //!< aspect ratio of window
        double lastMouseX;       //!< last mouse position x
        double lastMouseY;       //!< last mouse position y
        ObjectMoveMode moveMode; //!< current state of interaction

        // View
        glm::mat4 projMx;                          //!< Camera's projection matrix
        std::shared_ptr<Core::OrbitCamera> camera; //!< Camera's view matrix

        // Shader program
        std::unique_ptr<glowl::GLSLProgram> shaderQuad;
        std::unique_ptr<glowl::GLSLProgram> shaderSkybox;
        std::unique_ptr<glowl::GLSLProgram> shaderDome;
        std::unique_ptr<glowl::GLSLProgram> shaderParticleCPU;
        std::unique_ptr<glowl::GLSLProgram> shaderParticleGPU;
        std::unique_ptr<glowl::GLSLProgram> shaderParticleCompute; //!< Compute shader

        // Vertex buffer
        std::unique_ptr<glowl::Mesh> vaQuad;
        std::unique_ptr<glowl::Mesh> vaSkybox;
        std::unique_ptr<glowl::Mesh> vaDome;
        GLuint vaCPU;
        GLuint vaGPU;

        // Other buffer for CPU particles
        GLuint vboCPU;
        GLuint positionBufferCPU;

        // Other buffer for GPU particles
        GLuint vboGPU;
        GLuint ssboGPU;

        // FBO and its attachment
        GLuint fbo;           //!< handle for FBO
        GLuint fboTexColor;   //!< handle for color attachments
        GLuint fboTexId;      //!< handle for color attachments
        GLuint fboTexNormals; //!< handle for color attachments
        GLuint fboTexPos;     //!< handle for color attachments
        GLuint fboTexDepth;   //!< handle for depth buffer attachment

        // Texture
        std::shared_ptr<glowl::Texture2D> texBoard;     //!< board texture
        std::shared_ptr<glowl::Texture2D> texSphere;    //!< sphere texture
        std::shared_ptr<glowl::Texture2D> texPenguin;   //!< penguin texture
        std::shared_ptr<glowl::Texture2D> texDuck;      //!< duck texture
        std::shared_ptr<glowl::Texture2D> texBird;      //!< bird texture
        std::shared_ptr<glowl::Texture2D> texSnowflake; //!< snow texture atlas
        GLuint texSkybox;                               //!< cube texture

        // Object state
        std::vector<std::shared_ptr<Object>> objectList;

        // GUI variables
        int showFBOAtt;         //!< selector to show the different fbo attachments
        int showparticleMode;   //!< selector to show the different particles mode
        int maxParticles;       //!< maximun number of the particles
        float domeAlpha;     //!< transparency of the dome
        bool useDayNightCycle;
        
        float fovY;        //!< Camera's vertical field of view
        float zNear;       //!< near clipping plane
        float zFar;        //!< far clipping plane
        float lightLong;   //!< position of light, longitude in degree
        float lightLat;    //!< position of light, latitude in degree

        glm::vec3 ambientColor;
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float k_ambient;
        float k_diffuse;
        float k_specular;
        float k_exp;

        // Light
        float lightDist;        //!< distance from origin to light
        glm::vec3 lightPos;     //!< position of light
        
        // particles variable
        std::vector<Particle> particleContainer;        //!< store all particles for CPU particles
        std::vector<float> particlePosition;            //!< only store the particles for CPU particles which are alive and are in the dome
        std::vector<glm::vec4> particleContainerGPU;    //!< store all particles for GPU particles
        int drawParticleNum;                            //!< number of particles which are alive and are in the dome
        int lastshowparticleMode;
        int lastmaxParticles;
        int lastUsedParticle;
    };

} // namespace OGL4Core2::Plugins::PCVC::SnowGlobe

#endif // OGL4CORE2_PLUGINS_PCVC_SNOWGLOBE_SNOWGLOBE_H
