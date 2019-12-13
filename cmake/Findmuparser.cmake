find_path(muparser_INCLUDE_DIR muParserDef.h)
find_library(muparser_LIBRARY muparser)
mark_as_advanced(muparser_INCLUDE_DIR muparser_LIBRARY)

file(STRINGS "${muparser_INCLUDE_DIR}/muParserDef.h" muparser_version_str
     REGEX "^#define MUP_VERSION _T.*")
string(REGEX
       REPLACE "^#define MUP_VERSION _T..(.*).."
               "\\1"
               muparser_VERSION_STRING
               "${muparser_version_str}")
unset(muparser_version_str)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(muparser
                                  REQUIRED_VARS
                                  muparser_LIBRARY
                                  muparser_INCLUDE_DIR
                                  VERSION_VAR
                                  muparser_VERSION_STRING)

if(muparser_FOUND AND NOT TARGET muparser::muparser)
  add_library(muparser::muparser UNKNOWN IMPORTED)
  set_target_properties(muparser::muparser
                        PROPERTIES IMPORTED_LOCATION
                                   ${muparser_LIBRARY}
                                   INTERFACE_INCLUDE_DIRECTORIES
                                   ${muparser_INCLUDE_DIR})
endif()
