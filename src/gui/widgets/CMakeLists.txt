target_sources(
  gui
  PRIVATE plotwrapper.cpp
          qlabelslice.cpp
          qlabelmousetracker.cpp
          qplaintextmathedit.cpp)
add_subdirectory(ext/qcustomplot)
target_link_libraries(gui PUBLIC qcustomplot)
target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(
    gui_tests
    PUBLIC plotwrapper_t.cpp
           qlabelslice_t.cpp
           qlabelmousetracker_t.cpp
           qplaintextmathedit_t.cpp)
endif()
