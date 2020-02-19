find_path(tbb_INCLUDE_DIR tbb/tbb.h)
find_library(tbb_LIBRARY tbb)
mark_as_advanced(tbb_INCLUDE_DIR tbb_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(tbb
                                  REQUIRED_VARS
                                  tbb_LIBRARY
                                  tbb_INCLUDE_DIR)
if(tbb_FOUND AND NOT TARGET tbb::tbb)
  add_library(tbb::tbb UNKNOWN IMPORTED)
  set_target_properties(tbb::tbb
                        PROPERTIES IMPORTED_LOCATION
                                   ${tbb_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${tbb_INCLUDE_DIR})
endif()
