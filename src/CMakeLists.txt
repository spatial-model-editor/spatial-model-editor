if(BUILD_GUI)
  add_executable(spatial-model-editor)
  target_sources(spatial-model-editor PRIVATE spatial-model-editor.cpp icon.rc)
  if(WIN32)
    set_target_properties(spatial-model-editor PROPERTIES WIN32_EXECUTABLE TRUE)
  elseif(APPLE)
    set_target_properties(spatial-model-editor PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(spatial-model-editor PROPERTIES MACOSX_BUNDLE_ICONFILE
                                                          icon.icns)
  endif()
endif()

if(NOT SME_EXTERNAL_SMECORE)
  add_subdirectory(core)
endif()

if(BUILD_GUI)
  add_subdirectory(gui)
  target_compile_features(spatial-model-editor PRIVATE cxx_std_17)
  target_link_libraries(
    spatial-model-editor
    PRIVATE ${SME_EXTRA_GUI_LIBS}
            ${SME_EXTRA_EXE_LIBS}
            gui)
endif()
