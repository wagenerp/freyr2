
add_library(static-modules INTERFACE)

set(static-module-list  CACHE INTERNAL static-module-list FORCE)

function(add_module fn)

  get_filename_component(ident ${fn} NAME_WE)
  string(TOUPPER ${ident} IDENT)
  option(MODULE_${IDENT} "enable module ${fn}" ON)
  if (NOT MODULE_${IDENT})
    return()
  endif()

  if (OPT_DYNAMIC)
    add_library(module_${ident} SHARED ${fn})
    set_target_properties(module_${ident} PROPERTIES 
      PREFIX "" 
      SUFFIX "" 
      OUTPUT_NAME "${ident}.so")
    install(TARGETS module_${ident} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/freyr2)

    foreach(arg IN LISTS ARGN)
      target_link_libraries(module_${ident} ${arg})
    endforeach()
      target_link_libraries(module_${ident} alpha4c alpha4)
      set_property(TARGET module_${ident} PROPERTY POSITION_INDEPENDENT_CODE ON)

    target_compile_options(module_${ident} PRIVATE -Wno-pedantic)

  else()
    
    add_library(stmod_${ident} ${fn})
    if (OPT_DYNAMIC)
      set_property(TARGET stmod_${ident} PROPERTY POSITION_INDEPENDENT_CODE ON)
      endif()
    set_target_properties(stmod_${ident} PROPERTIES 
    PREFIX "" 
    SUFFIX "" 
    OUTPUT_NAME "stmod_${ident}.a")
    target_compile_options(stmod_${ident} PRIVATE -Wno-pedantic)
    string(REGEX REPLACE "[-]" "_" ident_sanitized "${ident}")
    foreach(arg IN LISTS ARGN) 
    target_link_libraries(stmod_${ident} ${arg})
    endforeach()
    if (NOT OPT_DYNAMIC)
    target_link_libraries(stmod_${ident} freyr2)
    endif()
    
    add_custom_command(
      TARGET stmod_${ident}
      POST_BUILD
      COMMAND ${CMAKE_OBJCOPY} 
      --redefine-sym describe=${ident_sanitized}_describe
      --redefine-sym init=${ident_sanitized}_init
      --redefine-sym deinit=${ident_sanitized}_deinit
      --redefine-sym iterate=${ident_sanitized}_iterate
      --redefine-sym flush=${ident_sanitized}_flush
      --redefine-sym mix=${ident_sanitized}_mix
      --redefine-sym leds_added=${ident_sanitized}_leds_added
      --redefine-sym leds_removed=${ident_sanitized}_leds_removed
      --redefine-sym SingletonInstance=${ident_sanitized}_SingletonInstance
      ${CMAKE_CURRENT_BINARY_DIR}/stmod_${ident}.a
      )
      
    list(APPEND static-module-list ${CMAKE_CURRENT_BINARY_DIR}/stmod_${ident}.a)
    set(static-module-list ${static-module-list} CACHE INTERNAL foo FORCE)
    target_link_libraries(static-modules INTERFACE stmod_${ident})
    
    add_dependencies(static-modules stmod_${ident})
  endif(OPT_DYNAMIC)

  
endfunction()


add_module(mod_bootstrap.cpp alpha4)
add_module(mod_grouping.cpp)
add_module(mod_coordinates.cpp)
add_module(mod_display.cpp alpha4 alpha4c)
add_module(mod_streams.cpp alpha4)

add_module(mod_filter_brightness.cpp alpha4 alpha4c)
add_module(mod_filter_overlay.cpp alpha4 alpha4c)


add_module(mod_input_stdin.cpp alpha4 alpha4c)
add_module(mod_mqtt.cpp alpha4 alpha4c mosquittopp)


add_module(anim_rainbow.c alpha4c)
add_module(anim_rainbow-s.c alpha4c)
add_module(anim_alert-s.c alpha4c)
add_module(anim_bifrost-s.c alpha4c)
add_module(anim_congress.c alpha4c)
add_module(anim_glimmer.c alpha4c)
add_module(anim_simplex-s.c alpha4c)
add_module(anim_propagator-s.c alpha4c)
add_module(anim_pulsar-s.c alpha4c)
add_module(anim_sparkle.c alpha4c)


add_module(egress_console.c alpha4c alpha4 )
add_module(egress_dummy.c)
add_module(egress_upsilon-striped.cpp)


add_module(blend_fade.c alpha4c)
add_module(blend_wipe.c alpha4c)



if(NOT OPT_DYNAMIC)
  set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/)

  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/static-module-registry.cpp
    COMMAND ${CMAKE_COMMAND} -E env NM=${CMAKE_NM} python3 
      ${TOOLS_DIR}/build-module-info.py 
      ${CMAKE_BINARY_DIR}/generated/static-module-registry.cpp
      ${static-module-list}
    DEPENDS static-modules ${static-module-list} ${TOOLS_DIR}/build-module-info.py 
  )
  add_library(static-module-registry
    ${CMAKE_BINARY_DIR}/generated/static-module-registry.cpp
  )
endif(NOT OPT_DYNAMIC)
