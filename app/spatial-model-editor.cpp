#include "mainwindow.hpp"
#include "sme/logger.hpp"
#include "sme/version.hpp"
#include <QApplication>
#include <QIcon>
#include <QVTKOpenGLNativeWidget.h>
#include <QtGui>
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  QString filename;
  if (argc > 1) {
    if (std::string arg = argv[1]; (arg == "-v") || (arg == "--version")) {
      fmt::print("{}\n", sme::common::SPATIAL_MODEL_EDITOR_VERSION);
      return 0;
    } else if ((arg == "-h") || (arg == "--help")) {
      fmt::print("Spatial Model Editor {}\n",
                 sme::common::SPATIAL_MODEL_EDITOR_VERSION);
      fmt::print("https://spatial-model-editor.readthedocs.io\n");
      return 0;
    } else {
      filename = argv[1];
    }
  }

  spdlog::set_pattern("%^[%L%$][%14s:%4#] %! :: %v%$");
  // set to lowest level here to show everything
  // then disable lower levels at compile time
  spdlog::set_level(spdlog::level::trace);

  // ensure our default opengl format is compatible with vtk
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  QApplication a(argc, argv);
  QApplication::setWindowIcon(QIcon(":/icon/icon.ico"));
  MainWindow w(filename);
  w.show();

  return QApplication::exec();
}
