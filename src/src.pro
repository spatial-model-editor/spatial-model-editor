TARGET = spatial-model-editor
TEMPLATE = app

include(core/core.pri)
include(gui/gui.pri)
include(../common.pri)
LIBS += -Lgui -lgui -Lcore -lcore
include(../ext/ext.pri)

SOURCES += spatial-model-editor.cpp
