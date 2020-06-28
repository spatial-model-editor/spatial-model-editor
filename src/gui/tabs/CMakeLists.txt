target_sources(
  gui
  PRIVATE tabfunctions.cpp
          tabgeometry.cpp
          tabparameters.cpp
          tabreactions.cpp
          tabsimulate.cpp
          tabspecies.cpp)
target_include_directories(gui PUBLIC .)

target_sources(
  gui_tests
  PUBLIC tabfunctions_t.cpp
         tabgeometry_t.cpp
         tabparameters_t.cpp
         tabreactions_t.cpp
         tabsimulate_t.cpp
         tabspecies_t.cpp)
