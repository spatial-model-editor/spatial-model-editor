QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spatial-model-editor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

# these static libraries are available pre-compiled
# from https://github.com/lkeegan/libsbml-static
LIBS += $$PWD/libsbml/lib/libsbml-static.a $$PWD/libsbml/lib/libexpat.a
INCLUDEPATH += $$PWD/libsbml/include

# on windows statically link to avoid missing dll errors:
win32: QMAKE_LFLAGS += -static-libgcc -static-libstdc++

# If you try to compile a PIE (as done by e.g. g++ on recent versions of ubuntu)
# it will cause a linker error as libexpat.a is not compiled with -fPIC
# uncomment the following to disable PIE compilation of the executable:
#QMAKE_LFLAGS += -no-pie
