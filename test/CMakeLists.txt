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
    src/test_numerics.cpp
    src/test_pde.cpp
    src/test_qlabelmousetracker.cpp
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

target_compile_features(tests PUBLIC cxx_std_17)

catch_discover_tests(tests
                     PROPERTIES
                     ENVIRONMENT
                     LSAN_OPTIONS=exitcode=0)

# add custom target to generate coverage report
add_custom_target(coverage
                  COMMAND ${CMAKE_COMMAND}
                          -E
                          make_directory
                          coverage
                  COMMAND lcov
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
                          -o
                          coverage/cov.info
                  COMMAND lcov
                          -q
                          -r
                          coverage/cov.info
                          */ext/*
                          -o
                          coverage/cov.info
                  COMMAND lcov
                          -q
                          -r
                          coverage/cov.info
                          */test/*
                          -o
                          coverage/cov.info
                  COMMAND lcov
                          -q
                          -r
                          coverage/cov.info
                          *_autogen/*
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
