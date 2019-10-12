find_path(sbml_INCLUDE_DIR sbml/SBMLDocument.h)
find_library(sbml_LIBRARY sbml-static)
mark_as_advanced(sbml_INCLUDE_DIR sbml_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sbml
                                  DEFAULT_MSG
                                  sbml_INCLUDE_DIR
                                  sbml_LIBRARY)

if(sbml_FOUND AND NOT TARGET sbml::sbml)
  add_library(sbml::sbml UNKNOWN IMPORTED)
  set_target_properties(sbml::sbml
                        PROPERTIES IMPORTED_LOCATION
                                   ${sbml_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${sbml_INCLUDE_DIR})
endif()
