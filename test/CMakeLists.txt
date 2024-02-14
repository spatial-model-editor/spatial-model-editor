add_executable(tests main.cpp)
target_link_libraries(
  tests
  PRIVATE core_tests
          $<$<BOOL:${SME_BUILD_GUI}>:gui_tests>
          $<$<BOOL:${SME_BUILD_CLI}>:cli_tests>
          ${SME_EXTRA_EXE_LIBS})
if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(tests)
endif()

if(SME_BUILD_GUI)
  target_compile_definitions(tests PRIVATE SME_ENABLE_GUI_TESTS)
endif()

find_package(
  Qt6
  REQUIRED
  CONFIG
  COMPONENTS Test)

add_library(testlib resources/test_resources.qrc)
if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(testlib)
endif()
target_link_libraries(
  testlib
  PUBLIC Catch2::Catch2
         sme::core
         Qt::Widgets
         Qt::Gui
         Qt::Core
         Qt::Test)
add_subdirectory(test_utils)

if(CMAKE_CXX_COMPILER_ID
   STREQUAL
   "Clang")
  file(WRITE ${PROJECT_BINARY_DIR}/coverage/llvm-cov-wrapper.sh "#!/bin/bash\n")
  file(APPEND ${PROJECT_BINARY_DIR}/coverage/llvm-cov-wrapper.sh
       "exec llvm-cov gcov \"$@\"\n")
  file(
    COPY ${PROJECT_BINARY_DIR}/coverage/llvm-cov-wrapper.sh
    DESTINATION ${PROJECT_BINARY_DIR}
    FILE_PERMISSIONS
      OWNER_EXECUTE
      OWNER_WRITE
      OWNER_READ)
  set(GCOV "${PROJECT_BINARY_DIR}/llvm-cov-wrapper.sh")
else()
  set(GCOV "gcov")
endif()

# ensure tests tagged with [gui] are not ran in parallel by ctest
catch_discover_tests(
  tests
  TEST_SPEC
  "[gui]"
  TEST_PREFIX
  "[gui]"
  PROPERTIES
  ENVIRONMENT
  ASAN_OPTIONS=halt_on_error=0
  RESOURCE_LOCK
  ResourceLockGui)
# also tag expensive tests so they can be easily excluded
catch_discover_tests(
  tests
  TEST_SPEC
  "[expensive]"
  TEST_PREFIX
  "[expensive]"
  PROPERTIES
  ENVIRONMENT
  ASAN_OPTIONS=halt_on_error=0)
catch_discover_tests(
  tests
  TEST_SPEC
  "~[gui]~[expensive]"
  PROPERTIES
  ENVIRONMENT
  ASAN_OPTIONS=halt_on_error=0)

# add custom target to generate coverage report
add_custom_target(
  clean-coverage
  COMMAND lcov -q -z -d .
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Removing all gcda files"
  VERBATIM)

# add custom target to generate coverage report
add_custom_target(
  coverage
  COMMAND ${CMAKE_COMMAND} -E make_directory coverage
  COMMAND lcov --gcov-tool ${GCOV} -q -d . -c -o coverage/cov.info
  COMMAND lcov -q -e coverage/cov.info */spatial-model-editor/src/* -o
          coverage/cov.info
  COMMAND lcov -l coverage/cov.info
  COMMAND genhtml -q coverage/cov.info -o coverage
  COMMAND xdg-open coverage/index.html
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Generating coverage report in coverage/index.html"
  VERBATIM)
