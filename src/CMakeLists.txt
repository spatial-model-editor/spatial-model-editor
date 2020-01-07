add_subdirectory(core)
add_subdirectory(gui)

add_executable(spatial-model-editor spatial-model-editor.cpp)
target_compile_features(spatial-model-editor PRIVATE cxx_std_17)
target_link_libraries(spatial-model-editor PRIVATE gui)

