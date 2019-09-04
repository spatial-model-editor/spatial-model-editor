TARGET = spatial-model-editor
TEMPLATE = app

include(common.pri)

# set logging level (at compile time)
CONFIG(release, debug|release):DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_TRACE"
CONFIG(debug, debug|release):DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_TRACE"

# disable qtDebug() output in release build
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

SOURCES += \
    src/main.cpp \

# for linux debug build, remove optimizations, add coverage & ASAN
CONFIG(debug, debug|release) {
    unix: QMAKE_CXXFLAGS_RELEASE -= -O2
    unix: QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
    unix: QMAKE_LFLAGS += -fsanitize=address -fno-omit-frame-pointer
}