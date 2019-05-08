#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "sbml" for configuration "Release"
set_property(TARGET sbml APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sbml PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "/usr/lib/x86_64-linux-gnu/libbz2.so;/usr/lib/x86_64-linux-gnu/libz.so;/usr/lib/x86_64-linux-gnu/libxml2.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libsbml.so.5.18.1"
  IMPORTED_SONAME_RELEASE "libsbml.so.5"
  )

list(APPEND _IMPORT_CHECK_TARGETS sbml )
list(APPEND _IMPORT_CHECK_FILES_FOR_sbml "${_IMPORT_PREFIX}/lib/libsbml.so.5.18.1" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
