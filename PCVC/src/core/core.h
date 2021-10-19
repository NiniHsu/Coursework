#ifndef OGL4CORE2_CORE_CORE_H
#define OGL4CORE2_CORE_CORE_H

#include <exception>
#include <filesystem>
#include <memory>
#include <vector>

// clang-format off
#include <glad/gl.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "util/fpscounter.h"
#include "input.h"
#include "camera/abstractcamera.h"

namespace OGL4Core2::Core {
    class RenderPlugin;

    class Core {
    public:
        Core();
        ~Core();

        void run();

        [[nodiscard]] std::filesystem::path getPluginResourcesPath() const;

        [[nodiscard]] bool isKeyPressed(Key key) const;
        [[nodiscard]] bool isMouseButtonPressed(MouseButton button) const;
        void getMousePos(double& xpos, double& ypos) const;

        void setWindowSize(int width, int height) const;

        void registerCamera(const std::shared_ptr<AbstractCamera>& camera) const;
        void removeCamera() const;

    private:
        void resizeEvent(int width, int height);
        void keyEvent(int key, int scancode, int action, int mods);
        void charEvent(unsigned int codepoint);
        void mouseButtonEvent(int button, int action, int mods);
        void mouseMoveEvent(double xpos, double ypos);
        void mouseScrollEvent(double xoffset, double yoffset);

        GLFWwindow* window_;
        bool running_;

        FpsCounter fps_;

        std::shared_ptr<RenderPlugin> currentPlugin_;
        std::filesystem::path currentPluginResourcesPath_;
        std::exception currentPluginResourcesPathException_;
        int currentPluginIdx_;
        int pluginSelectionIdx_;
        std::vector<char> pluginNamesImGui_;

        int windowWidth_;
        int windowHeight_;
        double mouseX_;
        double mouseY_;

        AbstractCamera::MouseControlMode cameraControlMode_;
        mutable std::weak_ptr<AbstractCamera> camera_;

        static void initGLFW();
        static void terminateGLFW();

        static int glfwReferenceCounter_;
    };
} // namespace OGL4Core2::Core

#endif // OGL4CORE2_CORE_CORE_H
