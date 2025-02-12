target_sources(
  gui
  PRIVATE plotwrapper.cpp
          qlabelslice.cpp
          qlabelmousetracker.cpp
          qplaintextmathedit.cpp
          qmeshrenderer.cpp
          qvoxelrenderer.cpp
          smevtkwidget.cpp)
target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(
    gui_tests
    PUBLIC plotwrapper_t.cpp
           qlabelslice_t.cpp
           qlabelmousetracker_t.cpp
           qplaintextmathedit_t.cpp
           qmeshrenderer_t.cpp
           qvoxelrenderer_t.cpp
           smevtkwidget_t.cpp)
endif()
