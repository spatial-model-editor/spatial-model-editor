QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = spatial-model-editor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/numerics.cpp \
    src/qlabelmousetracker.cpp \
    src/sbml.cpp \
    src/simulate.cpp \
    ext/qcustomplot/qcustomplot.cpp

HEADERS += \
    inc/mainwindow.h \
    inc/numerics.h \
    inc/qlabelmousetracker.h \
    inc/sbml.h \
    inc/simulate.h \
    ext/exprtk/exprtk.hpp \
    ext/qcustomplot/qcustomplot.h

FORMS += \
    ui/mainwindow.ui

INCLUDEPATH += inc ext/exprtk ext/qcustomplot

# these static libraries are available from
# from https://github.com/lkeegan/libsbml-static
LIBS += $$PWD/libsbml/lib/libsbml-static.a $$PWD/libsbml/lib/libexpat.a
INCLUDEPATH += $$PWD/libsbml/include

# on windows add flags to support large object files
# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
win32: QMAKE_CXXFLAGS += -Wa,-mbig-obj

# on windows statically link to avoid missing dll errors:
win32: QMAKE_LFLAGS += -static-libgcc -static-libstdc++

# on osx: set visibility to match setting used for compiling static Qt5 libs
mac: QMAKE_CXXFLAGS += -fvisibility=hidden
