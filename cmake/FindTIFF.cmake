find_path(TIFF_INCLUDE_DIR tiff.h)
find_library(TIFF_LIBRARY tiff)
mark_as_advanced(TIFF_INCLUDE_DIR TIFF_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TIFF
                                  DEFAULT_MSG
                                  TIFF_INCLUDE_DIR
                                  TIFF_LIBRARY)

if(TIFF_FOUND AND NOT TARGET TIFF::TIFF)
  add_library(TIFF::TIFF UNKNOWN IMPORTED)
  set_target_properties(TIFF::TIFF
                        PROPERTIES IMPORTED_LOCATION
                                   ${TIFF_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${TIFF_INCLUDE_DIR})
endif()
