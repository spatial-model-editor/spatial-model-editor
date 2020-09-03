target_sources(
  gui
  PRIVATE qlabelslice.cpp
          qlabelmousetracker.cpp
          qplaintextmathedit.cpp)
add_subdirectory(ext/qcustomplot)
target_link_libraries(gui PUBLIC qcustomplot)
target_include_directories(gui PUBLIC .)

target_sources(
  gui_tests
  PUBLIC qlabelslice_t.cpp
         qlabelmousetracker_t.cpp
         qplaintextmathedit_t.cpp)
