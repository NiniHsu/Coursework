cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

cmake_policy(SET CMP0091 NEW)

project(lodepng)

set(LODEPNG_HEADERS
  lodepng.h)
set(LODEPNG_SOURCES
  lodepng.cpp)

add_library(lodepng STATIC ${LODEPNG_SOURCES} ${LODEPNG_HEADERS})
set_target_properties(lodepng PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS lodepng DESTINATION "lib")
install(FILES ${LODEPNG_HEADERS} DESTINATION "include")
