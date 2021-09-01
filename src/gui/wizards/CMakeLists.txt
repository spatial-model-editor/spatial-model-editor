target_sources(gui PRIVATE ode_import_wizard.cpp)
target_include_directories(gui PUBLIC .)

if(BUILD_TESTING)
  target_sources(gui_tests PUBLIC ode_import_wizard_t.cpp)
endif()
