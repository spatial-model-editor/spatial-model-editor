target_sources(
  gui
  PRIVATE tabevents.cpp
          tabevents.ui
          tabfunctions.cpp
          tabfunctions.ui
          tabgeometry.cpp
          tabgeometry.ui
          tabparameters.cpp
          tabparameters.ui
          tabreactions.cpp
          tabreactions.ui
          tabsimulate.cpp
          tabsimulate.ui
          tabspecies.cpp
          tabspecies.ui)
target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(
    gui_tests
    PUBLIC tabevents_t.cpp
           tabfunctions_t.cpp
           tabgeometry_t.cpp
           tabparameters_t.cpp
           tabreactions_t.cpp
           tabsimulate_t.cpp
           tabspecies_t.cpp)
endif()
