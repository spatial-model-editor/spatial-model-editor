QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = spatial-model-editor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

# disable qtDebug() output in release build
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/geometry.cpp \
    src/mainwindow.cpp \
    src/numerics.cpp \
    src/qlabelmousetracker.cpp \
    src/sbml.cpp \
    src/simulate.cpp \
    ext/qcustomplot/qcustomplot.cpp

HEADERS += \
    inc/geometry.h \
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
LIBS += $$PWD/ext/libsbml/lib/libsbml-static.a $$PWD/ext/libsbml/lib/libexpat.a
INCLUDEPATH += $$PWD/ext/libsbml/include

# on windows add flags to support large object files
# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
win32: QMAKE_CXXFLAGS += -Wa,-mbig-obj

# on windows statically link to avoid missing dll errors:
win32: QMAKE_LFLAGS += -static-libgcc -static-libstdc++

# on osx: set visibility to match setting used for compiling static Qt5 libs
mac: QMAKE_CXXFLAGS += -fvisibility=hidden

# on linux, enable compiler warnings
unix: QMAKE_CXXFLAGS += -Wall -Wcast-align -Wconversion -Wdouble-promotion -Wextra -Wformat=2 -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wshadow -Wsign-conversion -Wunused -Wuseless-cast -Wpedantic
