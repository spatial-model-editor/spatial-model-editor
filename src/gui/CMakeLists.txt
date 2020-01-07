add_library(gui mainwindow.cpp mainwindow.hpp guiutils.cpp guiutils.hpp)
target_link_libraries(gui
                      PUBLIC qcustomplot
                             core
                             Qt5::Widgets
                             Qt5::PrintSupport)
target_include_directories(gui PUBLIC .)

add_subdirectory(widgets)
add_subdirectory(dialogs)
add_subdirectory(tabs)

set_property(TARGET gui PROPERTY CXX_STANDARD 17)
set_target_properties(gui PROPERTIES POSITION_INDEPENDENT_CODE ON)
