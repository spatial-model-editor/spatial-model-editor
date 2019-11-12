set(SRC_LIST
    boundary.cpp
    dialogabout.cpp
    dialoganalytic.cpp
    dialogconcentrationimage.cpp
    dialogimagesize.cpp
    dialogunits.cpp
    dune.cpp
    geometry.cpp
    mainwindow.cpp
    mesh.cpp
    numerics.cpp
    pde.cpp
    qlabelmousetracker.cpp
    sbml.cpp
    simulate.cpp
    symbolic.cpp
    tiff.cpp
    triangulate.cpp
    units.cpp
    utils.cpp
    ${PROJECT_BINARY_DIR}/src/version.cpp)

configure_file("${PROJECT_SOURCE_DIR}/src/version.cpp.in"
               "${PROJECT_BINARY_DIR}/src/version.cpp")

# note: have to explicitly add headers here for automoc to find them: (and .qrc
# for autorcc)
add_library(mainwindow STATIC
            ${SRC_LIST}
            ${PROJECT_SOURCE_DIR}/inc/mainwindow.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogabout.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialoganalytic.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogconcentrationimage.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogimagesize.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogunits.hpp
            ${PROJECT_SOURCE_DIR}/inc/qlabelmousetracker.hpp
            ${PROJECT_SOURCE_DIR}/ext/qcustomplot/qcustomplot.h
            ${PROJECT_SOURCE_DIR}/resources/resources.qrc)

target_include_directories(mainwindow PUBLIC ${PROJECT_SOURCE_DIR}/inc)

target_include_directories(mainwindow SYSTEM
                           PUBLIC ${PROJECT_SOURCE_DIR}/ext
                                  ${LLVM_INCLUDE_DIRS}
                                  ${PROJECT_SOURCE_DIR}/ext/qcustomplot
                                  ${PROJECT_SOURCE_DIR}/ext/triangle)

target_compile_features(mainwindow PUBLIC cxx_std_17)

target_compile_definitions(mainwindow PUBLIC $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>)
target_compile_definitions(mainwindow PUBLIC $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>)
target_link_options(mainwindow PUBLIC $<$<CONFIG:Debug>:--coverage -fsanitize=address -fno-omit-frame-pointer -Wall -Wextra -Wpedantic>)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs asmparser core executionengine instcombine mcjit native nativecodegen scalaropts vectorize support transformutils)

target_link_libraries(
  mainwindow
  PUBLIC ${PROJECT_SOURCE_DIR}/ext/libsbml/lib/libsbml-static.a
         expat::expat
         sbml::sbml
         dunecopasi::dunecopasi
         TIFF::TIFF
         muparser::muparser
         spdlog::spdlog
         symengine::symengine
         ${llvm_libs}
         ${PROJECT_SOURCE_DIR}/ext/qcustomplot/libqcustomplot.a
         ${PROJECT_SOURCE_DIR}/ext/triangle/triangle.o
         Qt5::Widgets
         Qt5::PrintSupport
         fmt::fmt
         GMP::GMP)

set_target_properties(mainwindow PROPERTIES POSITION_INDEPENDENT_CODE ON)
