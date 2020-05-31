add_library(core STATIC resources/resources.qrc)

add_subdirectory(mesh)
add_subdirectory(sbml)
add_subdirectory(simulate)

# set Logger level

target_compile_definitions(
  core PUBLIC
  SPDLOG_ACTIVE_LEVEL=$<$<CONFIG:Debug>:SPDLOG_LEVEL_TRACE>$<$<CONFIG:Release>:SPDLOG_LEVEL_INFO>
  ${SME_EXTRA_CORE_DEFS}
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
target_compile_options(core
                       PUBLIC $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang>>:-shared-libasan>)

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

# use TBB
if(WITH_TBB)
  find_package(tbb REQUIRED)
  target_compile_definitions(core PUBLIC SPATIAL_MODEL_EDITOR_USE_TBB)
  target_link_libraries(core PUBLIC tbb::tbb)
endif()

find_package(dunecopasi REQUIRED)
find_package(TIFF REQUIRED)
find_package(muparser REQUIRED)
find_package(spdlog REQUIRED)

target_sources(core
               PRIVATE
               duneini.cpp
               geometry.cpp
               pde.cpp
               symbolic.cpp
               tiff.cpp
               units.cpp
               utils.cpp
               ${PROJECT_BINARY_DIR}/src/core/version.cpp)

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
                             Qt5::Widgets
                             Qt5::Gui
                             Qt5::Core
                             ${SME_EXTRA_CORE_LIBS})
