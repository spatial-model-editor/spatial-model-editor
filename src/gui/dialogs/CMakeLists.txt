target_sources(
  gui
  PRIVATE dialogabout.cpp
          dialoganalytic.cpp
          dialogconcentrationimage.cpp
          dialogcoordinates.cpp
          dialogdisplayoptions.cpp
          dialogeditunit.cpp
          dialogexport.cpp
          dialogimagesize.cpp
          dialogimageslice.cpp
          dialogsimulationoptions.cpp
          dialogunits.cpp)
target_include_directories(gui PUBLIC .)

target_sources(
  gui_tests
  PUBLIC dialogabout_t.cpp
         dialoganalytic_t.cpp
         dialogconcentrationimage_t.cpp
         dialogcoordinates_t.cpp
         dialogdisplayoptions_t.cpp
         dialogeditunit_t.cpp
         dialogexport_t.cpp
         dialogimagesize_t.cpp
         dialogimageslice_t.cpp
         dialogsimulationoptions_t.cpp
         dialogunits_t.cpp)
