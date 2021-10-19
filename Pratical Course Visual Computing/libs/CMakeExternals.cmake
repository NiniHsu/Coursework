################################################################################
### CMakeExternals.cmake #######################################################
################################################################################

find_package(Git REQUIRED)

# Clone external script
if (NOT EXISTS "${CMAKE_BINARY_DIR}/script-externals")
  message(STATUS "Downloading external scripts")
  execute_process(COMMAND
    ${GIT_EXECUTABLE} clone -b v2.1 https://github.com/UniStuttgart-VISUS/megamol-cmake-externals.git script-externals --depth 1
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ()

# Include external script
include("${CMAKE_BINARY_DIR}/script-externals/cmake/External.cmake")

# Cleanup CMake UI
mark_as_advanced(FORCE FETCHCONTENT_BASE_DIR)
mark_as_advanced(FORCE FETCHCONTENT_FULLY_DISCONNECTED)
mark_as_advanced(FORCE FETCHCONTENT_QUIET)
mark_as_advanced(FORCE FETCHCONTENT_UPDATES_DISCONNECTED)

################################################################################
### Dependencies ###############################################################
################################################################################

##### GLFW #####

include(GNUInstallDirs)

if (WIN32)
  set(GLFW_LIB "${CMAKE_INSTALL_LIBDIR}/glfw3.lib")
else ()
  set(GLFW_LIB "${CMAKE_INSTALL_LIBDIR}/libglfw3.a")
endif ()

add_external_project(glfw STATIC
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG "3.3.2"
  BUILD_BYPRODUCTS "<INSTALL_DIR>/${GLFW_LIB}"
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS=OFF
    -DGLFW_BUILD_DOCS=OFF
    -DGLFW_BUILD_EXAMPLES=OFF
    -DGLFW_BUILD_TESTS=OFF
    -DGLFW_USE_HYBRID_HPG=ON
    -DUSE_MSVC_RUNTIME_LIBRARY_DLL=OFF
  FOLDER_NAME libs/external)

add_external_library(glfw3
  PROJECT glfw
  LIBRARY ${GLFW_LIB})

if (NOT WIN32)
  find_package(Threads)
  target_link_libraries(glfw3 INTERFACE rt m X11 Threads::Threads)
endif ()

##### glm #####

add_external_headeronly_project(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG "0.9.9.8")

##### glowl #####

add_external_headeronly_project(glowl
  GIT_REPOSITORY https://github.com/invor/glowl.git
  GIT_TAG "645e78d0824af893beba84e5dbcc01735d6358c0"
  INCLUDE_DIR "include")

##### imgui #####

if (WIN32)
  set(IMGUI_LIB "lib/imgui.lib")
else ()
  set(IMGUI_LIB "lib/libimgui.a")
endif ()

external_get_property(glfw INSTALL_DIR)

add_external_project(imgui STATIC
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG "v1.78"
  BUILD_BYPRODUCTS "<INSTALL_DIR>/${IMGUI_LIB}"
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_SOURCE_DIR}/libs/imgui/CMakeLists.txt"
    "<SOURCE_DIR>/CMakeLists.txt"
  DEPENDS
    glfw
  CMAKE_ARGS
    -DGLAD_INCLUDE_DIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}/libs/glad/include
    -DGLFW_INCLUDE_DIR:PATH=${INSTALL_DIR}/include
  FOLDER_NAME libs/external)

add_external_library(imgui
  LIBRARY ${IMGUI_LIB})

##### imGuIZMO.quat #####

if (WIN32)
  set(IMGUIZMO_LIB "lib/imguizmo.lib")
else ()
  set(IMGUIZMO_LIB "lib/libimguizmo.a")
endif ()

external_get_property(imgui INSTALL_DIR)
external_get_property(glm SOURCE_DIR)

add_external_project(imguizmo STATIC
  GIT_REPOSITORY https://github.com/BrutPitt/imGuIZMO.quat.git
  GIT_TAG "v3.0"
  BUILD_BYPRODUCTS "<INSTALL_DIR>/${IMGUIZMO_LIB}"
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy
  "${CMAKE_SOURCE_DIR}/libs/imguizmo/CMakeLists.txt"
  "<SOURCE_DIR>/CMakeLists.txt"
  DEPENDS imgui glm
  CMAKE_ARGS
    -DIMGUI_INCLUDE_DIR:PATH=${INSTALL_DIR}/include
    -DGLM_INCLUDE_DIR:PATH=${SOURCE_DIR}
  FOLDER_NAME libs/external)

add_external_library(imguizmo
  LIBRARY ${IMGUIZMO_LIB})

##### LodePNG #####

if (WIN32)
  set(LODEPNG_LIB "lib/lodepng.lib")
else ()
  set(LODEPNG_LIB "lib/liblodepng.a")
endif ()

add_external_project(lodepng STATIC
  GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
  GIT_TAG "34628e89e80cd007179b25b0b2695e6af0f57fac"
  BUILD_BYPRODUCTS "<INSTALL_DIR>/${LODEPNG_LIB}"
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_SOURCE_DIR}/libs/lodepng/CMakeLists.txt"
    "<SOURCE_DIR>/CMakeLists.txt"
  FOLDER_NAME libs/external)

add_external_library(lodepng
  LIBRARY ${LODEPNG_LIB})

##### datraw #####

add_external_headeronly_project(datraw
  GIT_REPOSITORY https://github.com/UniStuttgart-VISUS/datraw.git
  GIT_TAG "281001a962fd79484436e1ba6297359a8c3b346d"
  INCLUDE_DIR "datraw")
