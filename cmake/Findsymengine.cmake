find_path(symengine_INCLUDE_DIR symengine/symengine_config.h)
find_library(symengine_LIBRARY symengine)
mark_as_advanced(symengine_INCLUDE_DIR symengine_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(symengine
                                  DEFAULT_MSG
                                  symengine_INCLUDE_DIR
                                  symengine_LIBRARY)

if(symengine_FOUND AND NOT TARGET symengine::symengine)
  add_library(symengine::symengine UNKNOWN IMPORTED)
  set_target_properties(symengine::symengine
                        PROPERTIES IMPORTED_LOCATION
                                   ${symengine_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${symengine_INCLUDE_DIR})
endif()
