# compile tests
set(SRC_LIST
    src/main.cpp
    src/test_dialoganalytic.cpp
    src/test_dialogconcentrationimage.cpp
    src/test_dialogimagesize.cpp
    src/test_dialogunits.cpp
    src/test_dune.cpp
    src/test_geometry.cpp
    src/test_mainwindow.cpp
    src/test_mesh.cpp
    src/test_pde.cpp
    src/test_qlabelmousetracker.cpp
    src/test_qplaintextmathedit.cpp
    src/test_sbml.cpp
    src/test_simulate.cpp
    src/test_symbolic.cpp
    src/test_units.cpp
    src/test_utils.cpp)

add_executable(tests ${SRC_LIST} inc/qt_test_utils.hpp
                     ../resources/resources.qrc)

target_include_directories(tests PRIVATE inc ../ext/catch)

find_package(Qt5 COMPONENTS Test REQUIRED)

target_link_libraries(tests PRIVATE mainwindow Qt5::Test)

set_property(TARGET tests PROPERTY CXX_STANDARD 17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  file(WRITE ${PROJECT_BINARY_DIR}/coverage/llvm-cov-wrapper.sh "#!/bin/bash\n")
  file(APPEND ${PROJECT_BINARY_DIR}/coverage/llvm-cov-wrapper.sh
       "exec llvm-cov gcov \"$@\"\n")
  file(COPY ${PROJECT_BINARY_DIR}/coverage/llvm-cov-wrapper.sh
       DESTINATION ${PROJECT_BINARY_DIR}
       FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
  set(GCOV "${PROJECT_BINARY_DIR}/llvm-cov-wrapper.sh")
else()
  set(GCOV "gcov")
endif()

catch_discover_tests(tests
                     PROPERTIES
                     ENVIRONMENT
                     LSAN_OPTIONS=exitcode=0)

# add custom target to generate coverage report
add_custom_target(clean-coverage
                  COMMAND lcov
                          -q
                          -z
                          -d
                          .
                  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                  COMMENT "Removing all gcda files"
                  VERBATIM)

# add custom target to generate coverage report
add_custom_target(coverage
                  COMMAND ${CMAKE_COMMAND}
                          -E
                          make_directory
                          coverage
                  COMMAND lcov
                          --gcov-tool
                          ${GCOV}
                          -q
                          -d
                          .
                          -c
                          -o
                          coverage/cov.info
                  COMMAND lcov
                          -q
                          -r
                          coverage/cov.info
                          */usr/*
                          */ext/*
                          *_autogen/*
                          */test/*
                          -o
                          coverage/cov.info
                  COMMAND lcov -l coverage/cov.info
                  COMMAND genhtml
                          -q
                          coverage/cov.info
                          -o
                          coverage
                  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                  COMMENT "Generating coverage report in coverage/index.html"
                  VERBATIM)
