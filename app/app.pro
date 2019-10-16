TARGET = spatial-model-editor
TEMPLATE = app

include(../src/src.pri)
include(../common.pri)
win32 {
    CONFIG(release, debug|release):LIBS += -L../src/release -lmainwindow
    CONFIG(debug, debug|release):LIBS += -L../src/debug -lmainwindow
}
!win32: LIBS += -L../src -lmainwindow
include(../ext/ext.pri)

SOURCES += main.cpp
