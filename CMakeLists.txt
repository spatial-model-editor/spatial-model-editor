# require CMake version >= 3.13, and use highest available version
cmake_minimum_required(VERSION 3.13...3.17)
message(STATUS "CMake version ${CMAKE_VERSION}")

# version number here is embedded in compiled executable
project(SpatialModelEditor
        VERSION 0.8.5
        DESCRIPTION "Spatial Model Editor"
        LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# find required Qt5 modules
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
message(STATUS "Found Qt5Core: ${Qt5Core_location}")

# enable Qt5 utils: moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# hack to add missing windows libs by hand
set(SME_EXTRA_CORE_LIBS "" CACHE STRING "Optional additional core libs")
message(STATUS "SME_EXTRA_CORE_LIBS: ${SME_EXTRA_CORE_LIBS}")
set(SME_EXTRA_EXE_LIBS "" CACHE STRING "Optional additional exe libs")
message(STATUS "SME_EXTRA_EXE_LIBS: ${SME_EXTRA_EXE_LIBS}")
set(SME_EXTRA_CORE_DEFS "" CACHE STRING "Optional additional defines for core")
message(STATUS "SME_EXTRA_CORE_DEFS: ${SME_EXTRA_CORE_DEFS}")

# compile triangle library
add_subdirectory(ext/triangle)

add_subdirectory(ext/qcustomplot)

# compile spatial model editor
set(WITH_TBB yes CACHE BOOL "Build with TBB support (multithreading)")
add_subdirectory(src)

# compile spatial cli
set(BUILD_CLI yes CACHE BOOL "Build command-line version")
if(BUILD_CLI)
  add_subdirectory(cli)
endif()

# compile python library
set(BUILD_PYTHON_LIBRARY yes CACHE BOOL "Build python library")
if(BUILD_PYTHON_LIBRARY)
  set(PYBIND11_CPP_STANDARD -std=c++17)
  add_subdirectory(ext/pybind11)
  add_subdirectory(sme)
endif()

# compile tests
include(CTest)
include(Catch)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

# compile benchmarks
set(BUILD_BENCHMARKS yes CACHE BOOL "Build benchmarks")
if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()
