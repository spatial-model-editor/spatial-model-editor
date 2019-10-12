find_path(muparser_INCLUDE_DIR muParser.h)
find_library(muparser_LIBRARY muparser)
mark_as_advanced(muparser_INCLUDE_DIR muparser_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(muparser
                                  DEFAULT_MSG
                                  muparser_INCLUDE_DIR
                                  muparser_LIBRARY)

if(muparser_FOUND AND NOT TARGET muparser::muparser)
  add_library(muparser::muparser UNKNOWN IMPORTED)
  set_target_properties(muparser::muparser
                        PROPERTIES IMPORTED_LOCATION
                                   ${muparser_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${muparser_INCLUDE_DIR})
endif()
