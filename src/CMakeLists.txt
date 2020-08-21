add_executable(spatial-model-editor)
target_sources(spatial-model-editor PRIVATE spatial-model-editor.cpp)
add_subdirectory(core)
add_subdirectory(gui)
target_compile_features(spatial-model-editor PRIVATE cxx_std_17)
target_link_libraries(spatial-model-editor PRIVATE ${SME_EXTRA_EXE_LIBS} gui)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result AND NOT WIN32) # skip on windows due to mingw issues with FLTO
  message(STATUS "IPO enabled for spatial-model-editor")
  set_target_properties(spatial-model-editor
                        PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_target_properties(core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_target_properties(gui PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "IPO not supported")
endif()
