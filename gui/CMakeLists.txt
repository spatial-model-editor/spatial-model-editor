add_library(gui STATIC)
if(BUILD_TESTING)
  add_library(gui_tests STATIC)
endif()

target_sources(gui PRIVATE mainwindow.cpp guiutils.cpp)
target_include_directories(gui PUBLIC .)

find_package(qcustomplot)
target_link_libraries(gui PUBLIC qcustomplot::qcustomplot)

find_package(
  VTK REQUIRED
  COMPONENTS RenderingQt
             GUISupportQt
             CommonColor
             CommonCore
             CommonDataModel
             FiltersCore
             InteractionStyle
             RenderingUI
             RenderingContextOpenGL2
             RenderingCore
             RenderingFreeType
             RenderingGL2PSOpenGL2
             RenderingLOD
             RenderingOpenGL2
             RenderingVolume
             RenderingVolumeOpenGL2)
message(STATUS "VTK_LIBRARIES found: ${VTK_LIBRARIES}")

if(BUILD_TESTING)
  target_sources(gui_tests PUBLIC mainwindow_t.cpp guiutils_t.cpp)
  target_link_libraries(gui_tests PUBLIC gui testlib)
endif()

add_subdirectory(widgets)
add_subdirectory(dialogs)
add_subdirectory(tabs)
add_subdirectory(wizards)
add_subdirectory(rendering)

target_link_libraries(
  gui
  PUBLIC Qt::Widgets
         sme::core
         Qt::OpenGLWidgets
         Qt::OpenGL
         ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS
  gui
  MODULES
  ${VTK_LIBRARIES})
