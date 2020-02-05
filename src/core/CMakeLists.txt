add_library(core STATIC resources/resources.qrc)

# set Logger level

target_compile_definitions(
  core PUBLIC
  SPDLOG_ACTIVE_LEVEL=$<$<CONFIG:Debug>:SPDLOG_LEVEL_TRACE>$<$<CONFIG:Release>:SPDLOG_LEVEL_INFO>
  )

# set Compile options and warnings

set_target_properties(core PROPERTIES CXX_STANDARD 17)
set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_options(core
                    PUBLIC
                    $<$<CONFIG:Debug>:
                    --coverage
                    -fsanitize=address
                    -fno-omit-frame-pointer>
                    $<$<CXX_COMPILER_ID:Clang>:-fuse-ld=lld>)
target_compile_options(core
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

# find SymEngine and check it is compiled with LLVM enabled

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
    WARNING
      "Supplied SymEngine library does not appear to be compiled with LLVM support"
    )
endif()
target_include_directories(core SYSTEM PRIVATE ${SYMENGINE_INCLUDE_DIRS})
target_include_directories(core PUBLIC .)

# find libSBML and check it is compiled with the spatial extension enabled

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
    WARNING
      "Supplied libSBML library does not appear to be compiled with spatial extension support"
    )
endif()

find_package(dunecopasi REQUIRED)
find_package(TIFF REQUIRED)
find_package(muparser REQUIRED)
find_package(spdlog REQUIRED)

target_sources(core
               PRIVATE
               boundary.cpp
               duneini.cpp
               dunesim.cpp
               geometry.cpp
               mesh.cpp
               pde.cpp
               pixelsim.cpp
               sbml.cpp
               simulate.cpp
               symbolic.cpp
               tiff.cpp
               triangulate.cpp
               units.cpp
               utils.cpp
               ${PROJECT_BINARY_DIR}/src/core/version.cpp)

target_sources(core
               PRIVATE
               basesim.hpp
               boundary.hpp
               duneini.hpp
               dunesim.hpp
               geometry.hpp
               mesh.hpp
               pde.hpp
               pixelsim.hpp
               sbml.hpp
               simulate.hpp
               symbolic.hpp
               tiff.hpp
               triangulate.hpp
               units.hpp
               utils.hpp
               version.hpp)

configure_file("${PROJECT_SOURCE_DIR}/src/core/version.cpp.in"
               "${PROJECT_BINARY_DIR}/src/core/version.cpp")

target_link_libraries(core
                      PUBLIC sbml-static
                             dunecopasi::dunecopasi
                             TIFF::TIFF
                             muparser::muparser
                             spdlog::spdlog
                             ${SYMENGINE_LIBRARIES}
                             triangle
                             Qt5::Core
                             Qt5::Gui)
