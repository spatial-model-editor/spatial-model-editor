#include <QApplication>
#include <QtGui>

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>

#include "mainwindow.h"

LIBSBML_CPP_NAMESPACE_USE

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // some code that requires the libSBML spatial extension to compile:
    SpatialPkgNamespaces sbmlns(3,1,1);
    SBMLDocument document(&sbmlns);

    return a.exec();
}
