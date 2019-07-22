QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TEMPLATE = lib

CONFIG += staticlib
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
CONFIG += release

HEADERS += qcustomplot.h
SOURCES += qcustomplot.cpp
