add_library(gui STATIC)
if(BUILD_TESTING)
  add_library(gui_tests STATIC)
endif()

if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(gui)
  if(BUILD_TESTING)
    qt6_disable_unicode_defines(gui_tests)
  endif()
endif()

target_sources(gui PRIVATE mainwindow.cpp guiutils.cpp)
target_include_directories(gui PUBLIC .)

find_package(qcustomplot)
target_link_libraries(gui PUBLIC qcustomplot::qcustomplot)

if(BUILD_TESTING)
  target_sources(gui_tests PUBLIC mainwindow_t.cpp guiutils_t.cpp)
  target_link_libraries(gui_tests PUBLIC gui testlib Qt6::OpenGLWidgets Qt6::OpenGL OpenGL::GL OpenGL::GLU)
endif()

add_subdirectory(widgets)
add_subdirectory(dialogs)
add_subdirectory(tabs)
add_subdirectory(wizards)

target_link_libraries(gui PUBLIC Qt::Widgets sme::core Qt6::OpenGLWidgets Qt6::OpenGL OpenGL::GL OpenGL::GLU)
set_property(TARGET gui PROPERTY CXX_STANDARD 17)
set_target_properties(gui PROPERTIES POSITION_INDEPENDENT_CODE ON)
