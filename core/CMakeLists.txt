add_library(core STATIC mesh/include/sme/mesh3d.hpp)
add_library(
  sme::core
  ALIAS
  core)

# Install-related code

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/sme)

install(
  TARGETS core
  EXPORT core-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY common/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY mesh/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY model/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY simulate/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set_target_properties(core PROPERTIES EXPORT_NAME core)

install(
  EXPORT core-targets
  FILE smeTargets.cmake
  NAMESPACE sme::
  DESTINATION ${INSTALL_CONFIGDIR})

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/smeConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/smeConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/smeConfig.cmake
  INSTALL_DESTINATION ${INSTALL_CONFIGDIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/smeConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/smeConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR})

export(
  EXPORT core-targets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/smeTargets.cmake
  NAMESPACE sme::)

export(PACKAGE sme)

# End of install-related code

if(BUILD_TESTING)
  add_library(core_tests STATIC)
endif()

if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(core)
  if(BUILD_TESTING)
    qt6_disable_unicode_defines(core_tests)
  endif()
endif()

# find SymEngine and check it is compiled with LLVM enabled
find_package(
  SymEngine
  CONFIG
  REQUIRED)
message(STATUS "Found SymEngine library: ")
message(STATUS "Testing SymEngine LLVM & SBML support")
try_compile(
  SYMENGINE_LLVM "${CMAKE_CURRENT_BINARY_DIR}/cxx"
  "${CMAKE_SOURCE_DIR}/cmake/checkSymEngineLLVM.cpp"
  CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${SYMENGINE_INCLUDE_DIRS}
  LINK_LIBRARIES ${SYMENGINE_LIBRARIES}
  OUTPUT_VARIABLE SYMENGINE_LLVM_ERROR_LOG)
if(${SYMENGINE_LLVM})
  message(STATUS "Testing SymEngine LLVM & SBML support - found")
else()
  message(WARNING "SymEngine library is missing LLVM and/or SBML support")
  message(WARNING "${SYMENGINE_LLVM_ERROR_LOG}")
endif()

# find libSBML and check it is compiled with the spatial extension enabled
find_package(sbml-static REQUIRED)
message(STATUS "Testing libSBML spatial extension support")
try_compile(
  SBML_SPATIAL "${CMAKE_CURRENT_BINARY_DIR}/cxx"
  "${CMAKE_SOURCE_DIR}/cmake/checkSpatialSBML.cpp"
  LINK_LIBRARIES sbml-static
  OUTPUT_VARIABLE SBML_SPATIAL_ERROR_LOG)
if(${SBML_SPATIAL})
  message(STATUS "Testing libSBML spatial extension support - found")
else()
  message(WARNING "libSBML library is missing spatial extension support")
  message(WARNING "${SBML_SPATIAL_ERROR_LOG}")
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(spdlog REQUIRED)
find_package(TIFF REQUIRED)
find_package(fmt REQUIRED)
find_package(CGAL REQUIRED)
find_package(cereal REQUIRED)
find_package(Pagmo REQUIRED)
find_package(
  TBB
  CONFIG
  REQUIRED)
find_package(
  combine-static
  REQUIRED
  CONFIGS
  Combine-static-config.cmake)
target_include_directories(core SYSTEM PRIVATE ${SYMENGINE_INCLUDE_DIRS})
# set Logger level
target_compile_definitions(
  core PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${SME_LOG_LEVEL})

target_link_libraries(
  core
  PRIVATE TIFF::TIFF
          ${SYMENGINE_LIBRARIES}
          sbml-static
          Combine-static
          dune-copasi::dune-copasi
          CGAL::CGAL
          cereal::cereal
          TBB::tbb)
target_link_libraries(
  core
  PUBLIC opencv_core
         opencv_imgproc
         spdlog::spdlog
         Qt::Gui
         Qt::Core
         Pagmo::pagmo)

target_compile_definitions(core PUBLIC ${SME_EXTRA_CORE_DEFS})

if(BUILD_TESTING)
  target_link_libraries(
    core_tests
    PUBLIC dune-copasi::dune-copasi
           core
           testlib
           Qt6::OpenGLWidgets)
endif()

add_subdirectory(resources)
add_subdirectory(common)
add_subdirectory(mesh)
add_subdirectory(model)
add_subdirectory(simulate)

# set Compile options and warnings
set_target_properties(core PROPERTIES CXX_STANDARD 17)
set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)
