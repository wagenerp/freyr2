

if (OPT_DYNAMIC)
  message(STATUS "building freyr2 as shared library")
  set(freyr2_mode SHARED)
endif()

add_library(freyr2 ${freyr2_mode}
  types/stringlist.cpp
  core/animation.cpp
  core/basemodule.cpp
  core/egress.cpp
  core/frame.cpp
  core/module.cpp
)

if (OPT_DYNAMIC)
  target_compile_definitions(freyr2 PUBLIC -DOPT_DYNAMIC=1)
  set_property(TARGET freyr2 PROPERTY POSITION_INDEPENDENT_CODE ON)
else()

endif()
