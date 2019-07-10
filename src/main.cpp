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
  MainWindow w;
  w.show();

  return a.exec();
}
