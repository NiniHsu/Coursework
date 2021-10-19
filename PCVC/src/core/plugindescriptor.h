#ifndef OGL4CORE2_CORE_PLUGINDESCRIPTOR_H
#define OGL4CORE2_CORE_PLUGINDESCRIPTOR_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glad/gl.h>
#include <glowl/glowl.h>

#include "renderplugin.h"

namespace OGL4Core2::Core {
    class AbstractPluginDescriptor {
    public:
        explicit AbstractPluginDescriptor(std::string name, std::string path, int idx)
            : name_(std::move(name)), path_(std::move(path)), idx_(idx) {}
        virtual ~AbstractPluginDescriptor() = default;

        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& path() const { return path_; }
        [[nodiscard]] int idx() const { return idx_; }

        [[nodiscard]] virtual std::shared_ptr<RenderPlugin> create(const Core& c) const = 0;

    protected:
        std::string name_;
        std::string path_;
        int idx_;
    };

    template<class C>
    class PluginDescriptor : public AbstractPluginDescriptor {
    public:
        using AbstractPluginDescriptor::AbstractPluginDescriptor;
        [[nodiscard]] std::shared_ptr<RenderPlugin> create(const Core& c) const override {
            return std::make_shared<C>(c);
        }
    };
} // namespace OGL4Core2::Core

#endif // OGL4CORE2_CORE_PLUGINDESCRIPTOR_H
