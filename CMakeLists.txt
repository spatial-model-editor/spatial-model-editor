cmake_minimum_required(VERSION 3.13...3.22)
message(STATUS "CMake version ${CMAKE_VERSION}")

# version number here is embedded in compiled executable
project(
  SpatialModelEditor
  VERSION 1.2.1
  DESCRIPTION "Spatial Model Editor"
  LANGUAGES CXX)

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
set(CMAKE_CTEST_ARGUMENTS "-j${NPROC};--progress;-E;gui")

find_package(dune-copasi REQUIRED)

# find required Qt modules
option(
  SME_WITH_QT6
  "Use Qt6"
  ON)
if(SME_WITH_QT6)
  find_package(
    Qt6 QUIET
    COMPONENTS Core
               Gui
               Widgets)
endif()
if(NOT Qt6_FOUND)
  find_package(
    Qt5
    COMPONENTS Core
               Gui
               Widgets
    REQUIRED)
  get_target_property(
    QtCore_location
    Qt5::Core
    LOCATION)
  message(STATUS "Found QtCore: ${QtCore_location}")
else()
  get_target_property(
    QtCore_location
    Qt6::Core
    LOCATION)
  message(STATUS "Found QtCore: ${QtCore_location}")
endif()

# enable Qt utils: moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# stop Qt from defining UNICODE on windows to avoid dune issues: this used to be
# opt-in, but now done by default for Qt >= 6.1.2. See
# https://codereview.qt-project.org/c/qt/qtbase/+/350443
option(
  SME_QT_DISABLE_UNICODE
  "Disable Qt UNICODE option"
  OFF)

option(
  CGAL_Boost_USE_STATIC_LIBS
  "Enable when using static boost libraries"
  ON)
message(STATUS "CGAL_Boost_USE_STATIC_LIBS: ${CGAL_Boost_USE_STATIC_LIBS}")

set(SME_LOG_LEVEL
    "INFO"
    CACHE STRING
          "Logging level: OFF, CRITICAL, ERROR, WARN, INFO, DEBUG, TRACE")
message(STATUS "SME_LOG_LEVEL: ${SME_LOG_LEVEL}")

set(SME_EXTRA_EXE_LIBS
    ""
    CACHE STRING "Optional additional exe libs")
message(STATUS "SME_EXTRA_EXE_LIBS: ${SME_EXTRA_EXE_LIBS}")

set(SME_EXTRA_GUI_LIBS
    ""
    CACHE STRING "Optional additional gui libs")
message(STATUS "SME_EXTRA_GUI_LIBS: ${SME_EXTRA_GUI_LIBS}")

set(SME_EXTRA_CORE_DEFS
    ""
    CACHE STRING "Optional additional defines for core")
message(STATUS "SME_EXTRA_CORE_DEFS: ${SME_EXTRA_CORE_DEFS}")

option(
  SME_EXTERNAL_SMECORE
  "Use an externally provided sme::core instead of building it from source"
  OFF)
if(SME_EXTERNAL_SMECORE)
  find_package(
    sme
    REQUIRED
    CONFIG)
endif()

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

if(SME_BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Catch2 REQUIRED)
  include(Catch)
  add_subdirectory(test)
endif()

add_subdirectory(src)

if(SME_BUILD_CLI)
  add_subdirectory(ext/CLI11)
  add_subdirectory(cli)
endif()

# compile python library
if(SME_BUILD_PYTHON_LIBRARY)
  add_subdirectory(ext/pybind11)
  add_subdirectory(sme)
endif()
