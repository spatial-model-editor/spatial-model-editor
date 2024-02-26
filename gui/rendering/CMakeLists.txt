target_sources(
  gui
  PRIVATE Camera.cpp
          ShaderProgram.cpp
          Utils.cpp
          WireframeObjects.cpp
          ClipPlane.cpp)

target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(gui_tests PUBLIC)
endif()
