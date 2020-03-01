TEMPLATE = app
include(../src/core/core.pri)
include(../common.pri)
LIBS += -L../src/core -lcore
include(../ext/ext.pri)
CONFIG += console

TARGET = spatial-cli
SOURCES += spatial-cli.cpp
