target_sources(
  gui
  PRIVATE tabevents.cpp
          tabfunctions.cpp
          tabgeometry.cpp
          tabparameters.cpp
          tabreactions.cpp
          tabsimulate.cpp
          tabspecies.cpp)
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
