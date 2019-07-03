#include <QApplication>
#include <QtGui>

#include "mainwindow.h"

LIBSBML_CPP_NAMESPACE_USE

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  return a.exec();
}
