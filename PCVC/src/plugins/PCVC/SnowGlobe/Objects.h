#ifndef OGL4CORE2_PLUGINS_PCVC_SNOWGLOBE_OBJECTS_H
#define OGL4CORE2_PLUGINS_PCVC_SNOWGLOBE_OBJECTS_H

#include <memory>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glowl/glowl.h>

#include "core/renderplugin.h"

namespace OGL4Core2::Plugins::PCVC::SnowGlobe {
    class SnowGlobe;

    class Object {
    public:
        static glm::vec3 idToColor(unsigned int id);
        static unsigned int colorToId(const unsigned char col[3]);

        Object(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath);
        virtual ~Object() = default;

        int getId() const { return id; }

        void loadObj(const std::string& filename, std::unique_ptr<glowl::Mesh> &va);

        void draw(const glm::mat4& projMx, const glm::mat4& viewMx);

        virtual void reloadShaders() = 0;

        glm::mat4 modelMx;

    protected:
        SnowGlobe& basePlugin;
        int id;
        glm::vec3 idCol;
        std::unique_ptr<glowl::GLSLProgram> shaderProgram;
        std::unique_ptr<glowl::Mesh> va;
        std::shared_ptr<glowl::Texture2D> tex;
        std::string filepath;
    };

    class Base : public Object {
    public:
        Base(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath);

        void reloadShaders() override;

    private:
        void initShaders();
    };

    class Sphere : public Object {
    public:
        Sphere(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath);

        void reloadShaders() override;

    private:
        void initShaders();
    };

    class Birds : public Object {
    public:
        Birds(SnowGlobe& basePlugin, int id, std::shared_ptr<glowl::Texture2D> tex, std::string filepath);

        void reloadShaders() override;

    private:
        void initShaders();
    };

} // namespace OGL4Core2::Plugins::PCVC::SnowGlobe

#endif // OGL4CORE2_PLUGINS_PCVC_SNOWGLOBE_OBJECTS_H
