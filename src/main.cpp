#include <locale>

#include <QApplication>
#include <QtGui>

#include "mainwindow.hpp"
#include "version.hpp"

LIBSBML_CPP_NAMESPACE_USE

int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::string arg = argv[1];
    if ((arg == "-v") || (arg == "--version")) {
      std::cout << SPATIAL_MODEL_EDITOR_VERSION << std::endl;
      return 0;
    }
    if ((arg == "-h") || (arg == "--help")) {
      std::cout << "Spatial Model Editor " << SPATIAL_MODEL_EDITOR_VERSION
                << std::endl;
      std::cout << "https://www.github.com/lkeegan/spatial-model-editor"
                << std::endl;
      return 0;
    }
  }

  QApplication a(argc, argv);

  // Qt sets the locale according to the system one
  // This can cause problems with symengine
  // e.g. when a double is written as "3,142" vs "3.142"
  // it looks for "." to identify a double, but then uses strtod
  // to convert it, so "3.142" gets interpreted as "3"
  // Here we reset to the default "C" locale to avoid these issues
  std::locale::global(std::locale::classic());

  MainWindow w;
  w.show();

  return a.exec();
}
