cmake_minimum_required (VERSION 3.21)
project (freyr2)
include(GNUInstallDirs)

option(OPT_DYNAMIC "enable dynamic modules" ON)
option(OPT_DYNAMIC_MAIN "build freyr core as shared library" OFF)
option(OPT_GPROF "enable gprof output")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 23)
add_compile_options(-Wall -Wpedantic -Wextra -Werror)

if (OPT_GPROF) 
  add_compile_options(-pg)
  add_link_options(-pg)
endif()

include_directories(src/ alpha4/src/ unicorn/c/)

add_subdirectory(alpha4/src/)
add_subdirectory(unicorn/c/)


if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_compile_options(-Og -O0 -fsanitize=undefined)
  add_link_options(-fsanitize=undefined)
else()
  add_compile_options(-O3)
endif()

add_subdirectory(src/)
add_subdirectory(src/util)
# override default binary dir so that freyr can find dynamic modules out of the box
add_subdirectory(src/modules ${CMAKE_BINARY_DIR}/modules)

add_executable(freyr
  src/main/freyr.cpp
)


if (NOT OPT_DYNAMIC)
  target_link_libraries(freyr static-module-registry static-modules)
endif()

target_link_libraries(freyr
  freyr2
  freyr2util
  unicornc
  alpha4 
  alpha4c
)


install(TARGETS freyr)
if (OPT_DYNAMIC)
  set_target_properties(freyr PROPERTIES 
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBEXECDIR}/freyr2)
  install(TARGETS freyr2 freyr2util DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/freyr2)
endif()