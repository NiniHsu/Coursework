cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

cmake_policy(SET CMP0091 NEW)

project(glad)

add_library(glad
  include/glad/gl.h
  include/KHR/khrplatform.h
  src/gl.c)
target_include_directories(glad PUBLIC include/)
target_link_libraries(glad ${CMAKE_DL_LIBS})

set_target_properties(glad PROPERTIES
  FOLDER libs
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
