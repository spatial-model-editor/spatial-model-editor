find_library(dunecopasi_LIBRARY dune_copasi_lib)
find_path(dunecopasi_INCLUDE_DIR dune/logging.hh)
mark_as_advanced(dunecopasi_INCLUDE_DIR dunecopasi_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(dunecopasi
                                  DEFAULT_MSG
                                  dunecopasi_INCLUDE_DIR
                                  dunecopasi_LIBRARY)

if(dunecopasi_FOUND AND NOT TARGET dunecopasi::dunecopasi)
  add_library(dunecopasi::dunecopasi STATIC IMPORTED)
  foreach(DUNE_COPASI_DEPENDENCY
          dune-logging
          dunepdelab
          dunegrid
          dunegeometry
          ugS3
          ugS2
          ugL
          dunecommon)
    unset(tmpLIB CACHE)
    find_library(tmpLIB ${DUNE_COPASI_DEPENDENCY})
    target_link_libraries(dunecopasi::dunecopasi INTERFACE ${tmpLIB})
  endforeach()
  unset(tmpLIB CACHE)

  set_target_properties(dunecopasi::dunecopasi
                        PROPERTIES IMPORTED_LOCATION
                                   ${dunecopasi_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${dunecopasi_INCLUDE_DIR})

  target_compile_definitions(dunecopasi::dunecopasi
                             INTERFACE
                             DUNE_LOGGING_VENDORED_FMT=0
                             ENABLE_GMP=1
                             ENABLE_UG=1
                             MUPARSER_STATIC
                             UG_USE_NEW_DIMENSION_DEFINES)

endif()
