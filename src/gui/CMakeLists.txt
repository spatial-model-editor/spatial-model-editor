add_library(gui STATIC)
add_library(gui_tests STATIC)

target_sources(gui PRIVATE mainwindow.cpp guiutils.cpp)
target_include_directories(gui PUBLIC .)

add_subdirectory(widgets)
add_subdirectory(dialogs)
add_subdirectory(tabs)

target_link_libraries(gui PUBLIC Qt5::Widgets sme::core)
set_property(TARGET gui PROPERTY CXX_STANDARD 17)
set_target_properties(gui PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(BUILD_TESTING)
  target_sources(gui_tests PUBLIC mainwindow_t.cpp guiutils_t.cpp)
  target_link_libraries(gui_tests PUBLIC gui testlib)
endif()
