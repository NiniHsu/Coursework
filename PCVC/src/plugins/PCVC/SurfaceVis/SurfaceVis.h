#ifndef OGL4CORE2_PLUGINS_PCVC_SURFACEVIS_SURFACEVIS_H
#define OGL4CORE2_PLUGINS_PCVC_SURFACEVIS_SURFACEVIS_H

#include <memory>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glowl/glowl.h>

#include "core/camera/orbitcamera.h"
#include "core/pluginregister.h"
#include "core/renderplugin.h"

namespace OGL4Core2::Plugins::PCVC::SurfaceVis {

    class SurfaceVis : public Core::RenderPlugin {
        REGISTERPLUGIN(SurfaceVis, 104) // NOLINT

    public:
        static std::string name() { return "PCVC/SurfaceVis"; }

        explicit SurfaceVis(const Core::Core& c);
        ~SurfaceVis();

        void render() override;
        void resize(int width, int height) override;
        void keyboard(Core::Key key, Core::KeyAction action, Core::Mods mods) override;
        void mouseButton(Core::MouseButton button, Core::MouseButtonAction action, Core::Mods mods) override;
        void mouseMove(double xpos, double ypos) override;

    private:
        static glm::vec3 idToColor(unsigned int id);
        static unsigned int colorToId(const unsigned char col[3]);

        void renderGUI();

        void initShaders();
        void initVAs();

        void initFBO();
        void drawToFBO();

        void loadControlPoints(const std::string& filename);
        void saveControlPoints(const std::string& filename);

        // --------------------------------------------------------------------------------
        //  TODO: Define methods needed for surface/control point handling.
        // --------------------------------------------------------------------------------

        void initControlPoints();
        void initKnotVector();

        // Window state
        int wWidth;        //!< window width
        int wHeight;       //!< window height
        double lastMouseX; //!< last mouse position x
        double lastMouseY; //!< last mouse position y
        float zNear;
        float zFar;
        int pickedIdMove;

        // View
        std::shared_ptr<Core::OrbitCamera> camera; //!< view matrix
        glm::mat4 projMx;                          //!< projection matrix

        // GL objects
        std::unique_ptr<glowl::GLSLProgram> shaderQuad;           //!< shader program for window filling rectangle
        std::unique_ptr<glowl::GLSLProgram> shaderBox;            //!< shader program for box rendering
        std::unique_ptr<glowl::GLSLProgram> shaderControlPoints;  //!< shader program for control point rendering
        std::unique_ptr<glowl::GLSLProgram> shaderBSplineSurface; //!< shader program for b-spline surface rendering

        std::unique_ptr<glowl::Mesh> vaQuad;          //!< vertex array for window filling rectangle
        std::unique_ptr<glowl::Mesh> vaBox;           //!< vertex array for box
        std::unique_ptr<glowl::Mesh> vaControlPoints; //!< vertex array for control points
        std::unique_ptr<glowl::Mesh> vaControlPoints_LINES; //!< vertex array for control points
        std::vector<float> controlPointsVertices;
        std::vector<float> controlPointsColor;
        std::vector<GLuint> controlPointsIndices;
        GLuint vaEmpty;                               //!< vertex array for b-spline, not necessary
        GLuint ssbo;

        std::unique_ptr<glowl::FramebufferObject> fbo;

        // Other variables
        int maxTessGenLevel;
        int degree_p;
        int degree_q;

        // --------------------------------------------------------------------------------
        //  TODO: Define variables needed for surface generation/state.
        // --------------------------------------------------------------------------------

        // GUI variables
        float fovY;               //!< camera's vertical field of view
        bool showBox;             //!< toggle box drawing
        bool showNormals;         //!< toggle show normals
        bool useWireframe;        //!< toggle wireframe drawing
        int showControlPoints;    //!< toggle control point drawing
        float pointSize;          //!< point size of control points
        std::string dataFilename; //!< Filename for loading/storing data

        // --------------------------------------------------------------------------------
        //  TODO: Define GUI variables needed for surface:
        //    number of control points, picked control point, position, tessellation level
        // --------------------------------------------------------------------------------
        int numControlPoints_n;
        int numControlPoints_m;
        int numControlPoints; // numControlPoints_n * numControlPoints_m
        int pickedId;
        float pickedPosition[3];
        int tessLevelInner;
        int tessLevelOuter;

        glm::vec3 ambientColor;
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float k_ambient;
        float k_diffuse;
        float k_specular;
        float k_exp;
        int freq; //!< frequency of checkerboard texture
    };
} // namespace OGL4Core2::Plugins::PCVC::SurfaceVis

#endif // OGL4CORE2_PLUGINS_PCVC_SURFACEVIS_SURFACEVIS_H
