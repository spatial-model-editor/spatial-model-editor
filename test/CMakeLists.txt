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

# discover tests include(cmake/Catch.cmake) catch_discover_tests(tests)
