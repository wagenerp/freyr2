

if (OPT_DYNAMIC)
  message(STATUS "building freyr2util as shared library")
  set(freyr2util_mode SHARED)
endif()

add_library(freyr2util ${freyr2util_mode}
  sync.cpp
)

if (OPT_DYNAMIC)
  target_compile_definitions(freyr2util PUBLIC -DOPT_DYNAMIC=1)
else()

endif()
