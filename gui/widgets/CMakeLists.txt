target_sources(
  gui
  PRIVATE plotwrapper.cpp
          qlabelslice.cpp
          qlabelmousetracker.cpp
          qopenglmousetracker.cpp
          qplaintextmathedit.cpp
          qvoxelrenderer.cpp)
target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(
    gui_tests
    PUBLIC plotwrapper_t.cpp
           qlabelslice_t.cpp
           qlabelmousetracker_t.cpp
           qopenglmousetracker_t.cpp
           qplaintextmathedit_t.cpp
           qvoxelrenderer_t.cpp)
endif()
