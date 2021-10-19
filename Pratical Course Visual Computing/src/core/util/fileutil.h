#ifndef OGL4CORE2_CORE_FILEUTIL_H
#define OGL4CORE2_CORE_FILEUTIL_H

#include <filesystem>
#include <string>

namespace OGL4Core2::Core {
    class FileUtil {
    public:
        static std::filesystem::path getFullExeName();

        static std::filesystem::path findPluginResourcesPath(const std::string& path);
    };
} // namespace OGL4Core2::Core

#endif // OGL4CORE2_CORE_FILEUTIL_H
