TARGET = mainwindow
TEMPLATE = lib

QT += core gui
CONFIG += static

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

include(../topdir.pri)
include($$TOPDIR/common.pri)
include($$TOPDIR/ext/ext.pri)

SOURCES += \
    $$TOPDIR/src/boundary.cpp \
    $$TOPDIR/src/dialogabout.cpp \
    $$TOPDIR/src/dialogimagesize.cpp \
    $$TOPDIR/src/dialogunits.cpp \
    $$TOPDIR/src/dune.cpp \
    $$TOPDIR/src/geometry.cpp \
    $$TOPDIR/src/mainwindow.cpp \
    $$TOPDIR/src/mesh.cpp \
    $$TOPDIR/src/numerics.cpp \
    $$TOPDIR/src/pde.cpp \
    $$TOPDIR/src/qlabelmousetracker.cpp \
    $$TOPDIR/src/sbml.cpp \
    $$TOPDIR/src/simulate.cpp \
    $$TOPDIR/src/symbolic.cpp \
    $$TOPDIR/src/tiff.cpp \
    $$TOPDIR/src/triangulate.cpp \
    $$TOPDIR/src/utils.cpp \
    $$TOPDIR/src/version.cpp \

HEADERS += \
    $$TOPDIR/inc/boundary.hpp \
    $$TOPDIR/inc/dialogabout.hpp \
    $$TOPDIR/inc/dialogimagesize.hpp \
    $$TOPDIR/inc/dialogunits.hpp \
    $$TOPDIR/inc/dune.hpp \
    $$TOPDIR/inc/geometry.hpp \
    $$TOPDIR/inc/logger.hpp \
    $$TOPDIR/inc/mainwindow.hpp \
    $$TOPDIR/inc/mesh.hpp \
    $$TOPDIR/inc/numerics.hpp \
    $$TOPDIR/inc/pde.hpp \
    $$TOPDIR/inc/qlabelmousetracker.hpp \
    $$TOPDIR/inc/sbml.hpp \
    $$TOPDIR/inc/simulate.hpp \
    $$TOPDIR/inc/symbolic.hpp \
    $$TOPDIR/inc/tiff.hpp \
    $$TOPDIR/inc/triangulate.hpp \
    $$TOPDIR/inc/units.hpp \
    $$TOPDIR/inc/utils.hpp \
    $$TOPDIR/inc/version.hpp \

FORMS += \
    $$TOPDIR/ui/dialogabout.ui \
    $$TOPDIR/ui/dialogimagesize.ui \
    $$TOPDIR/ui/dialogunits.ui \
    $$TOPDIR/ui/mainwindow.ui \

RESOURCES += \
    $$TOPDIR/resources/resources.qrc \

INCLUDEPATH += \
    $$TOPDIR/inc \
    $$TOPDIR/ext \
