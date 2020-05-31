target_sources(
  gui
  PRIVATE dialogabout.cpp
          dialoganalytic.cpp
          dialogconcentrationimage.cpp
          dialogdisplayoptions.cpp
          dialogimagesize.cpp
          dialogimageslice.cpp
          dialogintegratoroptions.cpp
          dialogunits.cpp)
target_include_directories(gui PUBLIC .)

target_sources(
  gui_tests
  PUBLIC dialogabout_t.cpp
         dialoganalytic_t.cpp
         dialogconcentrationimage_t.cpp
         dialogdisplayoptions_t.cpp
         dialogimagesize_t.cpp
         dialogimageslice_t.cpp
         dialogintegratoroptions_t.cpp
         dialogunits_t.cpp)
