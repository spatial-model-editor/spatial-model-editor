# require CMake version >= 3.8, and use highest available version
cmake_minimum_required(VERSION 3.8...3.15)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()
message(STATUS "CMake version ${CMAKE_VERSION}")

# version number here is embedded in compiled executable
project(SpatialModelEditor
        VERSION 0.6.2
        DESCRIPTION "Spatial Model Editor"
        LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# this seems to be needed to pull in plugins for static builds todo: only do
# this for static qt5 library build
# add_definitions(-DQT_STATIC)

# find required Qt5 modules
find_package(Qt5 COMPONENTS Widgets Test PrintSupport REQUIRED)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
message(STATUS "Using Qt5Core from ${Qt5Core_location}")

# enable Qt5 utils: moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/ui)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# moc generates code in current dir:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(expat REQUIRED)
find_package(dunecopasi REQUIRED)
find_package(fmt REQUIRED)
find_package(GMP REQUIRED)
find_package(sbml REQUIRED)
find_package(TIFF REQUIRED)
find_package(LLVM 8 REQUIRED)
message(STATUS "LLVM version: ${LLVM_VERSION}")
find_package(muparser REQUIRED)
find_package(spdlog REQUIRED)
find_package(symengine REQUIRED)

enable_testing()

# compile mainwindow library
add_subdirectory(src)

# compile GUI application
add_subdirectory(app)

# compile tests
add_subdirectory(test)
