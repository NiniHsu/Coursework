cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

cmake_policy(SET CMP0091 NEW)

project(imgui)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(IMGUIZMO_HEADERS
  imGuIZMO.quat/imGuIZMOquat.h
  imGuIZMO.quat/imGuIZMOquat.h
  imGuIZMO.quat/vgConfig.h
  imGuIZMO.quat/vGizmo.h
  imGuIZMO.quat/vGizmoMath.h
  imGuIZMO.quat/vgMath.h)
set(IMGUIZMO_SOURCES
  imGuIZMO.quat/imGuIZMOquat.cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(imguizmo STATIC ${IMGUIZMO_SOURCES} ${IMGUIZMO_HEADERS})
target_compile_definitions(imguizmo PRIVATE VGIZMO_USES_GLM IMGUIZMO_IMGUI_FOLDER=./)
target_include_directories(imguizmo PRIVATE imGuIZMO.quat "${IMGUI_INCLUDE_DIR}" "${GLM_INCLUDE_DIR}")
set_target_properties(imguizmo PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS imguizmo DESTINATION "lib")
install(FILES ${IMGUIZMO_HEADERS} DESTINATION "include")
