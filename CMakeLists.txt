cmake_minimum_required(VERSION 3.13...3.21)
message(STATUS "CMake version ${CMAKE_VERSION}")

# version number here is embedded in compiled executable
project(
  SpatialModelEditor
  VERSION 1.1.5
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

find_package(dune-copasi REQUIRED)

# find required Qt modules
set(SME_WITH_QT6
    yes
    CACHE BOOL "Use Qt6")
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
set(SME_QT_DISABLE_UNICODE
    FALSE
    CACHE BOOL "Disable Qt UNICODE option")

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

set(SME_EXTERNAL_SMECORE
    no
    CACHE
      BOOL
      "Use an externally provided sme::core instead of building it from source")
if(SME_EXTERNAL_SMECORE)
  find_package(
    sme
    REQUIRED
    CONFIG)
endif()

set(BUILD_BENCHMARKS
    yes
    CACHE BOOL "Build benchmarks")
if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Catch2 REQUIRED)
  include(Catch)
  add_subdirectory(test)
endif()

set(SME_WITH_TBB
    no
    CACHE BOOL "Build with TBB support (multithreading)")
set(SME_WITH_OPENMP
    no
    CACHE BOOL "Build with OpenMP support (multithreading)")
if(SME_WITH_TBB AND SME_WITH_OPENMP)
  message(FATAL_ERROR "Cannot use both TBB and OpenMP together.")
endif()

set(BUILD_GUI
    yes
    CACHE BOOL "Build GUI version")
add_subdirectory(src)

# compile spatial cli
set(BUILD_CLI
    yes
    CACHE BOOL "Build command-line version")
if(BUILD_CLI)
  add_subdirectory(ext/CLI11)
  add_subdirectory(cli)
endif()

# compile python library
set(BUILD_PYTHON_LIBRARY
    yes
    CACHE BOOL "Build python library")
if(BUILD_PYTHON_LIBRARY)
  add_subdirectory(ext/pybind11)
  add_subdirectory(sme)
endif()
