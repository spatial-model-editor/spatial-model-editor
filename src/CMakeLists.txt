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
    pde.cpp
    qlabelmousetracker.cpp
    qplaintextmathedit.cpp
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

# note: have to add headers for automoc / autorcc to find them:
add_library(mainwindow STATIC
            ${SRC_LIST}
            ${PROJECT_SOURCE_DIR}/inc/mainwindow.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogabout.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialoganalytic.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogconcentrationimage.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogimagesize.hpp
            ${PROJECT_SOURCE_DIR}/inc/dialogunits.hpp
            ${PROJECT_SOURCE_DIR}/inc/qlabelmousetracker.hpp
            ${PROJECT_SOURCE_DIR}/inc/qplaintextmathedit.hpp
            ${PROJECT_SOURCE_DIR}/resources/resources.qrc)

target_include_directories(mainwindow PUBLIC ${PROJECT_SOURCE_DIR}/inc)

target_include_directories(mainwindow SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})

set_property(TARGET mainwindow PROPERTY CXX_STANDARD 17)

target_compile_definitions(
  mainwindow PUBLIC
  SPDLOG_ACTIVE_LEVEL=$<$<CONFIG:Debug>:SPDLOG_LEVEL_DEBUG>$<$<CONFIG:Release>:SPDLOG_LEVEL_INFO>
  )

target_link_options(mainwindow
                    PUBLIC
                    $<$<CONFIG:Debug>:
                    --coverage
                    -fsanitize=address
                    -fno-omit-frame-pointer>
                    $<$<CXX_COMPILER_ID:Clang>:-fuse-ld=lld>)
target_compile_options(mainwindow
                       PUBLIC $<$<CONFIG:Debug>:--coverage
                              -fsanitize=address
                              -fno-omit-frame-pointer
                              -Wall
                              -Wextra
                              -Wpedantic
                              -Wshadow
                              -Wunused
                              -Wconversion
                              -Wsign-conversion
                              -Wcast-align>)

# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs
                                asmparser
                                core
                                executionengine
                                instcombine
                                mcjit
                                native
                                nativecodegen
                                scalaropts
                                vectorize
                                support
                                transformutils)

target_link_libraries(mainwindow
                      PUBLIC sbml::sbml
                             expat::expat
                             dunecopasi::dunecopasi
                             TIFF::TIFF
                             muparser::muparser
                             spdlog::spdlog
                             symengine::symengine
                             ${llvm_libs}
                             qcustomplot
                             triangle
                             fmt::fmt
                             GMP::GMP)

set_target_properties(mainwindow PROPERTIES POSITION_INDEPENDENT_CODE ON)
