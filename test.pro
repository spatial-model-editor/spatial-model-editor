QT       += core gui testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = test
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    test/main.cpp \
    test/test_geometry.cpp \
    test/test_mainwindow.cpp \
    test/test_numerics.cpp \
    test/test_qlabelmousetracker.cpp \
    test/test_sbml.cpp \
    test/test_simulate.cpp \
    src/geometry.cpp \
    src/mainwindow.cpp \
    src/numerics.cpp \
    src/qlabelmousetracker.cpp \
    src/sbml.cpp \
    src/simulate.cpp \
    ext/qcustomplot/qcustomplot.cpp

HEADERS += \
    inc/mainwindow.h \
    inc/geometry.h \
    inc/numerics.h \
    inc/qlabelmousetracker.h \
    inc/sbml.h \
    inc/simulate.h \
    ext/exprtk/exprtk.hpp \
    ext/qcustomplot/qcustomplot.h \
    test/catch/catch.hpp \
    test/catch/catch_qt_ostream.hpp

FORMS += \
    ui/mainwindow.ui

INCLUDEPATH += inc ext/qcustomplot ext/exprtk test/catch

# for linux build, remove optimizations, add coverage info & compiler warnings
unix: QMAKE_CXXFLAGS_RELEASE -= -O2
unix: QMAKE_CXXFLAGS += --coverage
unix: QMAKE_LFLAGS += --coverage
unix: QMAKE_CXXFLAGS += -Wall -Wcast-align -Wconversion -Wdouble-promotion -Wextra -Wformat=2 -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wshadow -Wsign-conversion -Wunused -Wpedantic

# these static libraries are available from
# from https://github.com/lkeegan/libsbml-static
LIBS += $$PWD/ext/libsbml/lib/libsbml-static.a $$PWD/ext/libsbml/lib/libexpat.a
QMAKE_CXXFLAGS += -isystem $$PWD/ext/libsbml/include

# on windows add flags to support large object files
# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
win32: QMAKE_CXXFLAGS += -Wa,-mbig-obj

# on osx/linux, include QT headers as system headers to supress compiler warnings
mac|unix: QMAKE_CXXFLAGS += -isystem "$$[QT_INSTALL_HEADERS]/qt5" \
                        -isystem "$$[QT_INSTALL_HEADERS]/QtCore" \
                        -isystem "$$[QT_INSTALL_HEADERS]/QtWidgets" \
                        -isystem "$$[QT_INSTALL_HEADERS]/QtGui" \
                        -isystem "$$[QT_INSTALL_HEADERS]/QtTest"