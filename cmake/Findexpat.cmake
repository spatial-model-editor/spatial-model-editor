find_path(expat_INCLUDE_DIR expat.h)
find_library(expat_LIBRARY expat)
mark_as_advanced(expat_INCLUDE_DIR expat_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(expat
                                  DEFAULT_MSG
                                  expat_INCLUDE_DIR
                                  expat_LIBRARY)

if(expat_FOUND AND NOT TARGET expat::expat)
  add_library(expat::expat UNKNOWN IMPORTED)
  set_target_properties(expat::expat
                        PROPERTIES IMPORTED_LOCATION
                                   ${expat_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${expat_INCLUDE_DIR})
endif()
