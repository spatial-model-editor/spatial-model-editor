TARGET = spatial-model-editor
TEMPLATE = app

include(common.pri)

# disable (at compile time) TRACE and DEBUG spdlog logging
#DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_INFO"
DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_TRACE"

# disable qtDebug() output in release build
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

SOURCES += \
    src/main.cpp \
