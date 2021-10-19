#include "core.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "plugindescriptor.h"
#include "pluginregister.h"
#include "renderplugin.h"
#include "util/fileutil.h"
#include "util/glfwutil.h"
#include "util/glutil.h"

using namespace OGL4Core2::Core;

static constexpr int initWindowSizeWidth = 1280;
static constexpr int initWindowSizeHeight = 800;
static constexpr int openGLVersionMajor = 4;
static constexpr int openGLVersionMinor = 5;
static constexpr char title[] = "OGL4Core2";

Core::Core()
    : window_(nullptr),
      running_(false),
      currentPlugin_(nullptr),
      currentPluginIdx_(-1),
      pluginSelectionIdx_(0),
      windowWidth_(10),
      windowHeight_(10),
      mouseX_(0.0),
      mouseY_(0.0),
      cameraControlMode_(AbstractCamera::MouseControlMode::None) {
    Core::initGLFW();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, openGLVersionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, openGLVersionMinor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(initWindowSizeWidth, initWindowSizeHeight, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("GLFW window creation failed!");
    }

    glfwMakeContextCurrent(window_);

    int gladGLVersion = gladLoadGL(glfwGetProcAddress);
    if (gladGLVersion == 0) {
        throw std::runtime_error("Failed to initialize OpenGL context!");
    }

    std::cout << title << std::endl;
    GLUtil::printOpenGLInfo();

    if (!GLAD_GL_VERSION_4_5) {
        throw std::runtime_error("OpenGL context does not match requested version!");
    }

    // Set OpenGL error callback
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLUtil::OpenGLMessageCallback, nullptr);

    // Tell core about window size
    resizeEvent(initWindowSizeWidth, initWindowSizeHeight);

    glfwSetWindowUserPointer(window_, this);

    glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height) {
        static_cast<Core*>(glfwGetWindowUserPointer(window))->resizeEvent(width, height);
    });
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        static_cast<Core*>(glfwGetWindowUserPointer(window))->keyEvent(key, scancode, action, mods);
    });
    glfwSetCharCallback(window_, [](GLFWwindow* window, unsigned int codepoint) {
        static_cast<Core*>(glfwGetWindowUserPointer(window))->charEvent(codepoint);
    });
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* window, int button, int action, int mods) {
        static_cast<Core*>(glfwGetWindowUserPointer(window))->mouseButtonEvent(button, action, mods);
    });
    glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xpos, double ypos) {
        static_cast<Core*>(glfwGetWindowUserPointer(window))->mouseMoveEvent(xpos, ypos);
    });
    glfwSetScrollCallback(window_, [](GLFWwindow* window, double xoffset, double yoffset) {
        static_cast<Core*>(glfwGetWindowUserPointer(window))->mouseScrollEvent(xoffset, yoffset);
    });

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    const char* glsl_version = "#version 450";
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup Plugins
    if (PluginRegister::empty()) {
        throw std::runtime_error("No plugins found!");
    }
    // Plugin names for ImGui combo box
    for (const auto& pluginDescriptor : PluginRegister::getAll()) {
        const auto& name = pluginDescriptor->name();
        pluginNamesImGui_.insert(pluginNamesImGui_.end(), name.begin(), name.end());
        pluginNamesImGui_.push_back('\0');
    }
    pluginNamesImGui_.push_back('\0');

    // Plugins will be initialized on the fly in render method. No need to duplicate initialization here.
}

Core::~Core() {
    // Delete active plugin here, before destroying the OpenGL context.
    camera_.reset();
    currentPlugin_ = nullptr;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    Core::terminateGLFW();
}

void Core::run() {
    if (running_) {
        throw std::runtime_error("Core is already running!");
    }
    running_ = true;
    while (!glfwWindowShouldClose(window_)) {
        if (fps_.tick()) {
            std::string windowTitle = std::string(title) + " [ " + fps_.getFpsString() + " FPS ]";
            glfwSetWindowTitle(window_, windowTitle.c_str());
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10.0, 10.0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300.0, 600.0), ImGuiCond_Once);

        ImGui::Begin(title);

        if (ImGui::CollapsingHeader("Plugins", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Combo("Plugin", &pluginSelectionIdx_, pluginNamesImGui_.data());
        }
        if (currentPluginIdx_ != pluginSelectionIdx_) {
            currentPluginIdx_ = pluginSelectionIdx_;
            // Need to delete plugin first, so destructor of old plugin runs before constructor of new plugin.
            // Otherwise this could mess up OpenGL states.
            currentPlugin_ = nullptr;

            // Init new plugin
            const auto& plugin = PluginRegister::get(currentPluginIdx_);

            // Get plugin resource dir. This is done here, that we can keep access to path const as plugins should only
            // get a const reference to core. But as having a resource dir is optional for plugins, we want to show an
            // exception only if a plugin tries to access the path. Therefore catch exception an cache it.
            try {
                currentPluginResourcesPath_ = FileUtil::findPluginResourcesPath(plugin->path());
            } catch (const std::exception& ex) {
                currentPluginResourcesPathException_ = ex;
                currentPluginResourcesPath_.clear();
            }

            currentPlugin_ = plugin->create(*this);
            // Plugin needs to know window size.
            currentPlugin_->resize(windowWidth_, windowHeight_);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        if (currentPlugin_ != nullptr) {
            currentPlugin_->render();
        }

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
    running_ = false;
}

std::filesystem::path Core::getPluginResourcesPath() const {
    if (currentPluginResourcesPath_.empty()) {
        throw currentPluginResourcesPathException_;
    }
    return currentPluginResourcesPath_;
}

bool Core::isKeyPressed(Key key) const {
    return glfwGetKey(window_, static_cast<int>(key)) == GLFW_PRESS;
}

bool Core::isMouseButtonPressed(MouseButton button) const {
    return glfwGetMouseButton(window_, static_cast<int>(button)) == GLFW_PRESS;
}

void Core::getMousePos(double& xpos, double& ypos) const {
    glfwGetCursorPos(window_, &xpos, &ypos);
}

void Core::setWindowSize(int width, int height) const {
    glfwSetWindowSize(window_, width, height);
}

void Core::registerCamera(const std::shared_ptr<AbstractCamera>& camera) const {
    camera_ = camera;
}

void Core::removeCamera() const {
    camera_.reset();
}

void Core::resizeEvent(int width, int height) {
    // Assume Win32 or X11 system. According to GLFW docs window size to framebuffer size is 1:1 on this systems. We
    // use window and framebuffer size as the same value now. But here at least we check if they are really the same,
    // to throw at least a meaningful exception in case anybody is using a different system or GLFW behavior changes
    // in future.
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    if (width != framebufferWidth || height != framebufferHeight) {
        throw std::runtime_error("Window size and framebuffer size are not the same! "
                                 "You are probably using an unsupported system.");
    }

    // Save size for init of new plugin.
    windowWidth_ = width;
    windowHeight_ = height;
    if (currentPlugin_ != nullptr) {
        currentPlugin_->resize(width, height);
    }
}

void Core::keyEvent(int key, [[maybe_unused]] int scancode, int action, int mods) {
    mods = GLFWUtil::fixKeyboardMods(mods, key, action);
    if (!ImGui::GetIO().WantCaptureKeyboard && currentPlugin_ != nullptr) {
        currentPlugin_->keyboard(static_cast<Key>(key), static_cast<KeyAction>(action), Mods(mods));
    }
}

void Core::charEvent(unsigned int codepoint) {
    if (!ImGui::GetIO().WantTextInput && currentPlugin_ != nullptr) {
        currentPlugin_->charInput(codepoint);
    }
}

void Core::mouseButtonEvent(int button, int action, int mods) {
    auto b = static_cast<MouseButton>(button);
    auto a = static_cast<MouseButtonAction>(action);
    Mods m(mods);

    cameraControlMode_ = AbstractCamera::MouseControlMode::None;
    if (a == MouseButtonAction::Press && m.none()) {
        if (b == MouseButton::Left) {
            cameraControlMode_ = AbstractCamera::MouseControlMode::Left;
        } else if (b == MouseButton::Middle) {
            cameraControlMode_ = AbstractCamera::MouseControlMode::Middle;
        } else if (b == MouseButton::Right) {
            cameraControlMode_ = AbstractCamera::MouseControlMode::Right;
        }
    }

    if (!ImGui::GetIO().WantCaptureMouse && currentPlugin_ != nullptr) {
        currentPlugin_->mouseButton(b, a, m);
    }
}

void Core::mouseMoveEvent(double xpos, double ypos) {
    if (!ImGui::GetIO().WantCaptureMouse && currentPlugin_ != nullptr) {
        // Check camera event in mouseButtonEvent for correct modifier state. Checking here just for current key status
        // with glfwGetKey will miss the state when the modifier key was pressed before the window gets the focus. The
        // reason for this is, that glfwGetKey only returns a cached state, while the modifiers parameter contains the
        // live status.
        if (cameraControlMode_ != AbstractCamera::MouseControlMode::None) {
            auto camera = camera_.lock();
            if (camera) {
                double oldX = 2.0 * mouseX_ / static_cast<double>(windowWidth_) - 1.0;
                double oldY = 1.0 - 2.0 * mouseY_ / static_cast<double>(windowHeight_);
                double newX = 2.0 * xpos / static_cast<double>(windowWidth_) - 1.0;
                double newY = 1.0 - 2.0 * ypos / static_cast<double>(windowHeight_);
                camera->mouseMoveControl(cameraControlMode_, oldX, oldY, newX, newY);
            }
        }

        currentPlugin_->mouseMove(xpos, ypos);
    }
    mouseX_ = xpos;
    mouseY_ = ypos;
}

void Core::mouseScrollEvent(double xoffset, double yoffset) {
    if (!ImGui::GetIO().WantCaptureMouse && currentPlugin_ != nullptr) {
        if (!GLFWUtil::anyModKeyPressed(window_)) {
            auto camera = camera_.lock();
            if (camera) {
                camera->mouseScrollControl(xoffset, yoffset);
            }
        }
        currentPlugin_->mouseScroll(xoffset, yoffset);
    }
}

int Core::glfwReferenceCounter_ = 0;

void Core::initGLFW() {
    if (Core::glfwReferenceCounter_ <= 0) {
        glfwSetErrorCallback([](int error_code, const char* description) {
            std::cerr << "GLFW Error (" << error_code << "): " << description << std::endl;
        });
        if (!glfwInit()) {
            throw std::runtime_error("GLFW init failed!");
        }
    }
    Core::glfwReferenceCounter_++;
}

void Core::terminateGLFW() {
    Core::glfwReferenceCounter_--;
    if (Core::glfwReferenceCounter_ <= 0) {
        glfwTerminate();
        Core::glfwReferenceCounter_ = 0;
    }
}
