add_library(core STATIC)
add_library(core_tests STATIC)

# find SymEngine and check it is compiled with LLVM enabled
find_package(
  SymEngine
  CONFIG
  REQUIRED)
message(STATUS "Found SymEngine library: ")
message(STATUS "Testing SymEngine LLVM support")
try_compile(
  SYMENGINE_LLVM
  "${CMAKE_CURRENT_BINARY_DIR}/cxx"
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
  message(WARNING "${SYMENGINE_LLVM_ERROR_LOG}")
endif()

# find libSBML and check it is compiled with the spatial extension enabled
find_package(sbml-static REQUIRED)
message(STATUS "Testing libSBML spatial extension support")
try_compile(
  SBML_SPATIAL
  "${CMAKE_CURRENT_BINARY_DIR}/cxx"
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
  message(WARNING "${SBML_SPATIAL_ERROR_LOG}")
endif()

if(WIN32)
  set(OpenCV_STATIC ON) # temporary hack for windows CI build
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(spdlog REQUIRED)
find_package(TIFF REQUIRED)
find_package(fmt REQUIRED)
find_package(dunecopasi REQUIRED)
# dune-logging depends on fmt: temporary hack
target_link_libraries(dunecopasi::dunecopasi INTERFACE fmt::fmt)
find_package(muparser REQUIRED)
if(WITH_TBB)
  find_package(tbb REQUIRED)
  target_compile_definitions(core PUBLIC SPATIAL_MODEL_EDITOR_USE_TBB)
  target_link_libraries(core PRIVATE tbb::tbb)
endif()

target_include_directories(core SYSTEM PRIVATE ${SYMENGINE_INCLUDE_DIRS})
# set Logger level
target_compile_definitions(
  core
  PUBLIC
    SPDLOG_ACTIVE_LEVEL=$<$<CONFIG:Debug>:SPDLOG_LEVEL_TRACE>$<$<CONFIG:Release>:SPDLOG_LEVEL_INFO>
)

target_link_libraries(
  core
  PRIVATE TIFF::TIFF
          ${SYMENGINE_LIBRARIES}
          sbml-static
          dunecopasi::dunecopasi
          muparser::muparser)
target_link_libraries(
  core
  PUBLIC opencv_core
         opencv_imgproc
         spdlog::spdlog
         Qt5::Gui
         Qt5::Core
         resources
         ${SME_EXTRA_CORE_LIBS})

target_compile_definitions(core PUBLIC ${SME_EXTRA_CORE_DEFS})
target_link_libraries(
  core_tests
  PUBLIC core
         testlib
         dunecopasi::dunecopasi)

add_subdirectory(resources)
add_subdirectory(common)
add_subdirectory(mesh)
add_subdirectory(model)
add_subdirectory(simulate)

# set Compile options and warnings
set_target_properties(core PROPERTIES CXX_STANDARD 17)
set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# todo: do these in CI, not here:
target_link_options(
  core
  PUBLIC
  $<$<CONFIG:Debug>:
  --coverage
  -fsanitize=address
  -fno-omit-frame-pointer>
  $<$<CXX_COMPILER_ID:Clang>:-fuse-ld=lld>)
target_compile_options(
  core
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
target_compile_options(
  core
  PUBLIC $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang>>:-shared-libasan>)
