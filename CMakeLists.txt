cmake_minimum_required(VERSION 3.16...3.31)
message(STATUS "CMake version ${CMAKE_VERSION}")

# version number here is embedded in compiled executable
project(
  SpatialModelEditor
  VERSION 1.9.0
  DESCRIPTION "Spatial Model Editor"
  LANGUAGES C CXX)

# workaround for mingw gcc-14 c++20 issue:
# https://gitlab.kitware.com/cmake/cmake/-/issues/25974
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# workaround for CGAL using deprecated exec_program
if(POLICY CMP0153)
  cmake_policy(
    SET
    CMP0153
    OLD)
endif()

list(
  APPEND
  CMAKE_MODULE_PATH
  "${PROJECT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# "make test" excludes gui tests & uses all cores
include(ProcessorCount)
ProcessorCount(NPROC)
set(CMAKE_CTEST_ARGUMENTS "-j${NPROC};--progress;--output-on-failure;-E;gui")

# find required Qt modules
find_package(
  Qt6
  CONFIG
  REQUIRED
  COMPONENTS Core
             Gui
             Widgets
             OpenGLWidgets
             OpenGL)
get_target_property(
  QtCore_location
  Qt6::Core
  LOCATION)
message(STATUS "Found QtCore: ${QtCore_location}")

# enable Qt utils: moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(
  SME_USE_STATIC_LIBS
  "Use static versions of dependencies"
  ON)
if(SME_USE_STATIC_LIBS)
  set(CGAL_Boost_USE_STATIC_LIBS ON)
  set(OpenCV_STATIC ON)
endif()

set(SME_LOG_LEVEL
    "INFO"
    CACHE STRING
          "Logging level: OFF, CRITICAL, ERROR, WARN, INFO, DEBUG, TRACE")

set(SME_EXTRA_EXE_LIBS
    ""
    CACHE STRING "Optional additional exe libs")

set(SME_EXTRA_GUI_LIBS
    ""
    CACHE STRING "Optional additional gui libs")

set(SME_EXTRA_CORE_DEFS
    ""
    CACHE STRING "Optional additional defines for core")

option(
  SME_BUILD_CORE
  "Build core sme library"
  ON)
option(
  SME_BUILD_BENCHMARKS
  "Build benchmarks"
  ON)
option(
  SME_BUILD_GUI
  "Build GUI version"
  ON)
option(
  SME_BUILD_CLI
  "Build command-line version"
  ON)
option(
  SME_BUILD_PYTHON_LIBRARY
  "Build python library"
  ON)

if(NOT SME_BUILD_CORE)
  find_package(
    sme
    REQUIRED
    CONFIG)
  get_target_property(
    SME_CORE_LOCATION
    sme::core
    IMPORTED_LOCATION_RELEASE)
  message(STATUS "Found sme::core: ${SME_CORE_LOCATION}")
endif()

if(SME_BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Catch2 REQUIRED)
  include(Catch)
  add_subdirectory(test)
endif()

if(SME_BUILD_CORE)
  add_subdirectory(core)
endif()

if(SME_BUILD_GUI)
  add_subdirectory(gui)
  add_subdirectory(app)
endif()

if(SME_BUILD_CLI)
  add_subdirectory(ext/CLI11)
  add_subdirectory(cli)
endif()

# compile python library
if(SME_BUILD_PYTHON_LIBRARY)
  add_subdirectory(ext/nanobind)
  add_subdirectory(sme)
endif()

# display sme configuration options
message(STATUS "SME_BUILD_CORE: ${SME_BUILD_CORE}")
message(STATUS "SME_BUILD_GUI: ${SME_BUILD_GUI}")
message(STATUS "SME_BUILD_CLI: ${SME_BUILD_CLI}")
message(STATUS "SME_BUILD_PYTHON_LIBRARY: ${SME_BUILD_PYTHON_LIBRARY}")
message(STATUS "SME_BUILD_BENCHMARKS: ${SME_BUILD_BENCHMARKS}")
message(STATUS "SME_LOG_LEVEL: ${SME_LOG_LEVEL}")
message(STATUS "SME_EXTRA_CORE_DEFS: ${SME_EXTRA_CORE_DEFS}")
message(STATUS "SME_EXTRA_GUI_LIBS: ${SME_EXTRA_GUI_LIBS}")
message(STATUS "SME_EXTRA_EXE_LIBS: ${SME_EXTRA_EXE_LIBS}")
message(STATUS "SME_USE_STATIC_LIBS: ${SME_USE_STATIC_LIBS}")
