find_package(SymEngine CONFIG REQUIRED)
message(STATUS "Testing SymEngine LLVM support")
try_compile(SYMENGINE_LLVM "${CMAKE_CURRENT_BINARY_DIR}/cxx"
            "${CMAKE_SOURCE_DIR}/cmake/checkSymEngineLLVM.cpp"
            CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${SYMENGINE_INCLUDE_DIRS}
            LINK_LIBRARIES ${SYMENGINE_LIBRARIES}
            OUTPUT_VARIABLE SYMENGINE_LLVM_ERROR_LOG)
if(${SYMENGINE_LLVM})
  message(STATUS "Testing SymEngine LLVM support - found")
else()
  message(
    FATAL_ERROR
      "Supplied SymEngine library is not compiled with LLVM support")
endif()

#message(STATUS "SYMENGINE_INCLUDE_DIRS ${SYMENGINE_INCLUDE_DIRS}")
#message(STATUS "SYMENGINE_LIBRARIES ${SYMENGINE_LIBRARIES}")

find_package(dunecopasi REQUIRED)

find_package(sbml-static REQUIRED)
message(STATUS "Testing libSBML spatial extension support")
try_compile(SBML_SPATIAL "${CMAKE_CURRENT_BINARY_DIR}/cxx"
            "${CMAKE_SOURCE_DIR}/cmake/checkSpatialSBML.cpp"
            LINK_LIBRARIES sbml-static
            OUTPUT_VARIABLE SBML_SPATIAL_ERROR_LOG)
if(${SBML_SPATIAL})
  message(STATUS "Testing libSBML spatial extension support - found")
else()
  message(
    FATAL_ERROR
      "Supplied libSBML library is not compiled with spatial extension support")
endif()

find_package(TIFF REQUIRED)
find_package(muparser REQUIRED)
find_package(spdlog REQUIRED)

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

target_include_directories(mainwindow SYSTEM PUBLIC ${SYMENGINE_INCLUDE_DIRS})

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

target_link_libraries(mainwindow
                      PUBLIC sbml-static
                             dunecopasi::dunecopasi
                             TIFF::TIFF
                             muparser::muparser
                             spdlog::spdlog
                             ${SYMENGINE_LIBRARIES}
                             qcustomplot
                             triangle)

set_target_properties(mainwindow PROPERTIES POSITION_INDEPENDENT_CODE ON)
