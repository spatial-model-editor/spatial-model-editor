QT       += core gui testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = test
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    test/src/main.cpp \
    test/src/test_dune.cpp \
    test/src/test_geometry.cpp \
    test/src/test_mainwindow.cpp \
    test/src/test_mesh.cpp \
    test/src/test_numerics.cpp \
    test/src/test_qlabelmousetracker.cpp \
    test/src/test_reactions.cpp \
    test/src/test_sbml.cpp \
    test/src/test_simulate.cpp \
    test/src/test_symbolic.cpp \
    src/dune.cpp \
    src/geometry.cpp \
    src/mainwindow.cpp \
    src/mesh.cpp \
    src/numerics.cpp \
    src/qlabelmousetracker.cpp \
    src/reactions.cpp \
    src/sbml.cpp \
    src/simulate.cpp \
    src/symbolic.cpp \
    src/triangle_wrapper.cpp \
    src/version.cpp

HEADERS += \
    inc/mainwindow.hpp \
    inc/qlabelmousetracker.hpp \
    test/inc/catch.hpp \
    test/inc/qt_test_utils.hpp \
    test/inc/sbml_test_data/ABtoC.hpp \
    test/inc/sbml_test_data/very_simple_model.hpp \
    test/inc/sbml_test_data/yeast_glycolysis.hpp

FORMS += \
    ui/mainwindow.ui

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += inc ext test/inc

# for linux test build, remove optimizations, add coverage info & compiler warnings
unix: QMAKE_CXXFLAGS_RELEASE -= -O2
unix: QMAKE_CXXFLAGS += --coverage
unix: QMAKE_LFLAGS += --coverage
unix: QMAKE_CXXFLAGS += -Wall -Wcast-align -Wconversion -Wdouble-promotion -Wextra -Wformat=2 -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wshadow -Wsign-conversion -Wunused -Wpedantic

# these static libraries are available pre-compiled from
# from https://github.com/lkeegan/libsbml-static
LIBS += \
    $$PWD/ext/libsbml/lib/libsbml-static.a \
    $$PWD/ext/libsbml/lib/libexpat.a \
    $$PWD/ext/symengine/lib/libsymengine.a \
    $$PWD/ext/gmp/lib/libgmp.a

# these static libraries should be compiled first in their directories
LIBS += \
    $$PWD/ext/qcustomplot/libqcustomplot.a \
    $$PWD/ext/triangle/triangle.o

# on windows add flags to support large object files
# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
win32: QMAKE_CXXFLAGS += -Wa,-mbig-obj

# on osx: set visibility to match setting used for compiling static Qt5 libs
mac: QMAKE_CXXFLAGS += -fvisibility=hidden

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$$[QT_INSTALL_HEADERS]/qt5" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtCore" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtWidgets" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtGui" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtTest" \
    -isystem "$$PWD/ext/libsbml/include" \
    -isystem "$$PWD/ext/symengine/include" \
    -isystem "$$PWD/ext/gmp/include" \
    -isystem "$$PWD/ext/qcustomplot" \
    -isystem "$$PWD/ext/triangle"
