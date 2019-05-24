QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spatial-model-editor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/sbml.cpp \
    src/qlabelmousetracker.cpp \
    src/numerics.cpp

HEADERS += \
    inc/mainwindow.h \
    inc/sbml.h \
    inc/qlabelmousetracker.h \
    inc/numerics.h \
    ext/exprtk/exprtk.hpp

FORMS += \
    ui/mainwindow.ui

INCLUDEPATH += inc ext/exprtk

# these static libraries are available from
# from https://github.com/lkeegan/libsbml-static
LIBS += $$PWD/libsbml/lib/libsbml-static.a $$PWD/libsbml/lib/libexpat.a
INCLUDEPATH += $$PWD/libsbml/include

# on windows statically link to avoid missing dll errors:
win32: QMAKE_LFLAGS += -static-libgcc -static-libstdc++
