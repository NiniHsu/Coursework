cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# Policies
cmake_policy(SET CMP0091 NEW)

# Disable in source build
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Project
project(OGL4Core2
  VERSION 0.3.0)

# Set a default build type if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# Modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
include(resources)

# Dependencies
include("libs/CMakeExternals.cmake")

add_subdirectory(libs/glad/)

find_package(OpenGL REQUIRED)

# Core source files
set(core_source_files
  src/main.cpp
  src/core/core.cpp
  src/core/renderplugin.cpp
  src/core/camera/orbitcamera.cpp
  src/core/camera/trackball.cpp
  src/core/util/fileutil.cpp
  src/core/util/fpscounter.cpp)

# Core header files
set(core_header_files
  src/core/core.h
  src/core/input.h
  src/core/plugindescriptor.h
  src/core/pluginregister.h
  src/core/renderplugin.h
  src/core/camera/abstractcamera.h
  src/core/camera/orbitcamera.h
  src/core/camera/trackball.h
  src/core/util/fileutil.h
  src/core/util/fpscounter.h
  src/core/util/glfwutil.h
  src/core/util/glutil.h
  src/core/util/imguiutil.h)

# Find all plugin files
file(GLOB_RECURSE plugin_source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.cpp")
file(GLOB_RECURSE plugin_header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.h")
file(GLOB_RECURSE plugin_shader_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.tesc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.tese"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.geom"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.comp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/*.glsl")

# List of all files, we need to add all files (including shaders) to the target to show them within Visual Studio.
set(all_source_files
  ${core_source_files}
  ${core_header_files}
  ${plugin_source_files}
  ${plugin_header_files}
  ${plugin_shader_files})

# Determine plugin path relative to plugins dir and define it as PLUGIN_DIR per file for plugin registration.
foreach (file ${plugin_source_files} ${plugin_header_files})
  get_filename_component(file_path ${file} DIRECTORY)
  file(RELATIVE_PATH file_path_rel "/src/plugins" "/${file_path}") # Fake absolute path by prefixing "/".
  set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS -DPLUGIN_DIR=\\\"${file_path_rel}\\\")
endforeach ()

# Setup Visual Studio file tree
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${all_source_files})

# Disable extra console window on Windows.
set(WIN32_FLAG "")
if (MSVC)
  option(OGL4CORE2_DISABLE_CONSOLE "Disable extra console window." OFF)
  if (OGL4CORE2_DISABLE_CONSOLE)
    set(WIN32_FLAG "WIN32")
  endif ()
endif (MSVC)

# Project
add_executable(${PROJECT_NAME} ${WIN32_FLAG}
  ${all_source_files})
target_compile_definitions(${PROJECT_NAME} PRIVATE
  IMGUI_IMPL_OPENGL_LOADER_GLAD2
  VGIZMO_USES_GLM
  IMGUIZMO_IMGUI_FOLDER=./
  GLOWL_NO_ARB_BINDLESS_TEXTURE)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_EXTENSIONS OFF
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glad glfw3 glm glowl imgui imguizmo lodepng datraw)
if (UNIX)
  target_link_libraries(${PROJECT_NAME} PRIVATE "stdc++fs")
endif ()
if (MSVC AND OGL4CORE2_DISABLE_CONSOLE)
  target_link_options(${PROJECT_NAME} PRIVATE "/entry:mainCRTStartup")
endif ()

# Setup resources path
set(plugins_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins")

find_resource_dirs(res_dirs ${plugins_source_dir})

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.config" "PLUGINS_SOURCE_DIR=${plugins_source_dir}\n")

# Install
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

foreach (dir ${res_dirs})
  get_filename_component(dir_clean "${dir}" DIRECTORY)
  install(DIRECTORY "${plugins_source_dir}/${dir}/" DESTINATION "resources/${dir_clean}")
endforeach ()

# Package
set(OGL4CORE2_CPACK_GENERATOR "ZIP" CACHE STRING "Choose CPack generator.")
if (UNIX)
  set_property(CACHE OGL4CORE2_CPACK_GENERATOR PROPERTY STRINGS "TGZ" "ZIP" "STGZ" "DEB")
else ()
  set_property(CACHE OGL4CORE2_CPACK_GENERATOR PROPERTY STRINGS "ZIP" "NSIS")
endif ()

set(CPACK_GENERATOR "${OGL4CORE2_CPACK_GENERATOR}")
set(CPACK_PACKAGE_VENDOR "Visualization Research Center (VISUS) - University of Stuttgart")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OGL4Core2 is a framework to ease the development of prototypical or education-oriented graphical algorithms.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if (OGL4CORE2_CPACK_GENERATOR STREQUAL "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Visualization Research Center (VISUS) - University of Stuttgart")
  set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
  # https://stackoverflow.com/a/7363073
  set(CPACK_SET_DESTDIR true)
  set(CPACK_INSTALL_PREFIX /opt/ogl4core2)
elseif (OGL4CORE2_CPACK_GENERATOR STREQUAL "NSIS")
  # Start menu entry
  set(CPACK_PACKAGE_EXECUTABLES "OGL4Core2;OGL4Core2")
endif ()

# TODO: configure source packages

include(CPack)

# Enable strict warnings
option(OGL4CORE2_STRICT_WARNINGS "Enable strict compiler warnings." OFF)
if (OGL4CORE2_STRICT_WARNINGS)
  if (MSVC)
    # TODO: For now do not set "/WX" flag, as external libs generate warnings, which we cannot fix.
    # MSVC has an experimental feature to disable external warnings, see https://devblogs.microsoft.com/cppblog/broken-warnings-theory/.
    # Maybe in future there is cmake support for this feature, see https://gitlab.kitware.com/cmake/cmake/-/issues/17904.
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wall -Wextra -pedantic -pedantic-errors)
  endif ()
endif ()

# Visual Studio folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if (MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif ()
