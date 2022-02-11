target_sources(
  gui
  PRIVATE dialogabout.cpp
          dialoganalytic.cpp
          dialogconcentrationimage.cpp
          dialogcoordinates.cpp
          dialogdisplayoptions.cpp
          dialogeditunit.cpp
          dialogexport.cpp
          dialoggeometryimage.cpp
          dialogimage.cpp
          dialogimageslice.cpp
          dialogmeshingoptions.cpp
          dialogsimulationoptions.cpp
          dialogunits.cpp)
target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(
    gui_tests
    PUBLIC dialogabout_t.cpp
           dialoganalytic_t.cpp
           dialogconcentrationimage_t.cpp
           dialogcoordinates_t.cpp
           dialogdisplayoptions_t.cpp
           dialogeditunit_t.cpp
           dialogexport_t.cpp
           dialoggeometryimage_t.cpp
           dialogimage_t.cpp
           dialogimageslice_t.cpp
           dialogmeshingoptions_t.cpp
           dialogsimulationoptions_t.cpp
           dialogunits_t.cpp)
endif()
