add_executable(spatial-model-editor main.cpp ../resources/resources.qrc)
target_compile_features(spatial-model-editor PRIVATE cxx_std_17)
target_link_libraries(spatial-model-editor PRIVATE mainwindow)