# require CMake version >= 3.13, and use highest available version
cmake_minimum_required(VERSION 3.13...3.16)
message(STATUS "CMake version ${CMAKE_VERSION}")

# version number here is embedded in compiled executable
project(SpatialModelEditor
        VERSION 0.8.4
        DESCRIPTION "Spatial Model Editor"
        LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# find required Qt5 modules

# NOTE: this seems to be needed to pull in plugins for static builds
# add_definitions(-DQT_STATIC)
find_package(Qt5 COMPONENTS Widgets Test PrintSupport REQUIRED)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
message(STATUS "Found Qt5Core: ${Qt5Core_location}")

# enable Qt5 utils: moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# compile triangle library
add_subdirectory(ext/triangle)

# compile qcustomplot library
add_subdirectory(ext/qcustomplot)

# compile spatial model editor
set(WITH_TBB yes CACHE BOOL "Build with TBB support (multithreading)")
add_subdirectory(src)

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
