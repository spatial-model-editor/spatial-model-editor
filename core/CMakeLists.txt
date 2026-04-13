add_library(core STATIC)
target_compile_features(core PUBLIC cxx_std_20)
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

if(WIN32)
  set(SME_SBML_PACKAGE libsbml-static)
  set(SME_SBML_TARGET libsbml-static)
  set(SME_COMBINE_PACKAGE libCombine-static)
  set(SME_COMBINE_TARGET libCombine-static)
  set(SME_COMBINE_CONFIG libCombine-static-config.cmake)
else()
  set(SME_SBML_PACKAGE sbml-static)
  set(SME_SBML_TARGET sbml-static)
  set(SME_COMBINE_PACKAGE combine-static)
  set(SME_COMBINE_TARGET Combine-static)
  set(SME_COMBINE_CONFIG Combine-static-config.cmake)
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
  OUTPUT_VARIABLE
    SYMENGINE_LLVM_ERROR_LOG
    CXX_STANDARD
    20
    CXX_STANDARD_REQUIRED
    TRUE)
if(${SYMENGINE_LLVM})
  message(STATUS "Testing SymEngine LLVM & SBML support - Success")
else()
  message(
    FATAL_ERROR
      "SymEngine library is missing LLVM and/or SBML support:\n${SYMENGINE_LLVM_ERROR_LOG}"
  )
endif()

# find libSBML and check it is compiled with the spatial extension enabled
find_package(
  ${SME_SBML_PACKAGE}
  CONFIG
  REQUIRED)
if(WIN32)
  set_property(
    TARGET ${SME_SBML_TARGET}
    APPEND
    PROPERTY INTERFACE_COMPILE_DEFINITIONS
             LIBSBML_STATIC=1
             LIBLAX_STATIC=1)
endif()
message(STATUS "Testing libSBML spatial extension support")
try_compile(
  SBML_SPATIAL "${CMAKE_CURRENT_BINARY_DIR}/cxx"
  "${CMAKE_SOURCE_DIR}/cmake/checkSpatialSBML.cpp"
  LINK_LIBRARIES ${SME_SBML_TARGET}
  OUTPUT_VARIABLE
    SBML_SPATIAL_ERROR_LOG
    CXX_STANDARD
    20
    CXX_STANDARD_REQUIRED
    TRUE)
if(${SBML_SPATIAL})
  message(STATUS "Testing libSBML spatial extension support - Success")
else()
  message(
    FATAL_ERROR
      "libSBML library is missing spatial extension support:\n${SBML_SPATIAL_ERROR_LOG}"
  )
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(spdlog REQUIRED)
find_package(TIFF REQUIRED)
find_package(fmt REQUIRED)
if(WIN32)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()
find_package(CGAL REQUIRED)
find_package(cereal REQUIRED)
find_package(Pagmo REQUIRED)
find_package(
  TBB
  CONFIG
  REQUIRED)
find_package(
  ${SME_COMBINE_PACKAGE}
  REQUIRED
  CONFIGS
  ${SME_COMBINE_CONFIG})
if(WIN32)
  set_property(
    TARGET ${SME_COMBINE_TARGET}
    APPEND
    PROPERTY INTERFACE_COMPILE_DEFINITIONS LIBCOMBINE_STATIC=1)
endif()
if(SME_WITH_CUDA)
  find_package(CUDAToolkit REQUIRED)
  target_include_directories(core SYSTEM PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
  target_link_libraries(
    core
    PUBLIC ${CMAKE_DL_LIBS}
           CUDA::nvrtc_static
           CUDA::nvrtc_builtins_static
           CUDA::nvptxcompiler_static)
else()
  message(STATUS "CUDA toolkit disabled on this platform")
endif()
target_include_directories(core SYSTEM PRIVATE ${SYMENGINE_INCLUDE_DIRS})
# set Logger level
target_compile_definitions(
  core PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${SME_LOG_LEVEL})

find_package(dune-copasi REQUIRED)
target_link_libraries(
  core
  PRIVATE TIFF::TIFF
          ${SYMENGINE_LIBRARIES}
          ${SME_SBML_TARGET}
          ${SME_COMBINE_TARGET}
          CGAL::CGAL
          cereal::cereal
          TBB::tbb
          Dune::Copasi
          $<$<PLATFORM_ID:Windows>:psapi>)

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
    PUBLIC Dune::Copasi
           core
           testlib)
endif()

add_subdirectory(resources)
add_subdirectory(common)
add_subdirectory(mesh)
add_subdirectory(model)
add_subdirectory(simulate)
