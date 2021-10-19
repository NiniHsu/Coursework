cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

cmake_policy(SET CMP0091 NEW)

project(imgui)

set(IMGUI_PUBLIC_HEADERS
  imconfig.h
  imgui.h)
set(IMGUI_PRIVATE_HEADERS
  imgui_internal.h
  imstb_rectpack.h
  imstb_textedit.h
  imstb_truetype.h)
set(IMGUI_SOURCES
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_widgets.cpp)
set(IMGUI_IMPL_HEADERS
  examples/imgui_impl_opengl3.h
  examples/imgui_impl_glfw.h
  misc/cpp/imgui_stdlib.h)
set(IMGUI_IMPL_SOURCES
  examples/imgui_impl_opengl3.cpp
  examples/imgui_impl_glfw.cpp
  misc/cpp/imgui_stdlib.cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_IMPL_SOURCES} ${IMGUI_PUBLIC_HEADERS} ${IMGUI_PRIVATE_HEADERS} ${IMGUI_IMPL_HEADERS})
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)
target_include_directories(imgui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp
  ${GLAD_INCLUDE_DIR}
  ${GLFW_INCLUDE_DIR})
set_target_properties(imgui PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS imgui DESTINATION "lib")
install(FILES ${IMGUI_PUBLIC_HEADERS} ${IMGUI_PRIVATE_HEADERS} ${IMGUI_IMPL_HEADERS} DESTINATION "include")
