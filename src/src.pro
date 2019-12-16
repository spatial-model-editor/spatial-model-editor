TARGET = spatial-model-editor
TEMPLATE = app

include(core/core.pri)
include(gui/gui.pri)
include(../common.pri)
win32 {
    CONFIG(release, debug|release):LIBS += -Lgui/release -lgui -Lcore/release -lcore
    CONFIG(debug, debug|release):LIBS += -Lgui/debug -lgui -Lcore/debug -lcore
}
!win32:
LIBS += -Lgui -lgui -Lcore -lcore
include(../ext/ext.pri)

SOURCES += spatial-model-editor.cpp
