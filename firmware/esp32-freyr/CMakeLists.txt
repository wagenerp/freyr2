cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 23)

set(MODULE_MOD_INPUT_STDIN CACHE INTERNAL OFF)

set(EXTRA_COMPONENT_DIRS
    $ENV{IDF_PATH}/examples/common_components/led_strip
    )

include_directories(${CMAKE_BINARY_DIR}/generated)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(freyr2)

add_custom_target(generatedFiles)

include(main/device.cmake)

include_directories(../../alpha4/src ../../unicorn/c ../../src)
add_subdirectory(../../alpha4/src ${CMAKE_BINARY_DIR}/alpha4)
add_subdirectory(../../unicorn/c ${CMAKE_BINARY_DIR}/unicorn)
add_subdirectory(../../src ${CMAKE_BINARY_DIR}/freyr/core)
add_subdirectory(../../src/modules ${CMAKE_BINARY_DIR}/freyr/modules)


add_dependencies(freyr2.elf generatedFiles)

target_link_libraries(freyr2.elf PUBLIC 
  static-module-registry static-modules
  freyr2 unicornc alpha4 alpha4c
)

