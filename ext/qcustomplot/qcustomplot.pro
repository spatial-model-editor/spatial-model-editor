QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TEMPLATE = lib

CONFIG += staticlib
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
CONFIG += release
DESTDIR = $${PWD}

# on osx: set visibility to match setting used for compiling static Qt5 libs
mac: QMAKE_CXXFLAGS += -fvisibility=hidden

HEADERS += qcustomplot.h
SOURCES += qcustomplot.cpp
