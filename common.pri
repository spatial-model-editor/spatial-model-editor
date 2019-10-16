DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

# set logging level (at compile time)
CONFIG(release, debug|release):DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_INFO"
CONFIG(debug, debug|release):DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_DEBUG"

# on linux, enable GCC compiler warnings
unix: QMAKE_CXXFLAGS += -Wall -Wcast-align -Wconversion -Wdouble-promotion -Wextra -Wformat=2 -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wshadow -Wsign-conversion -Wunused -Wpedantic

# on windows statically link libgcc & libstdc++ to avoid missing dll errors:
win32: QMAKE_LFLAGS += -static-libgcc -static-libstdc++

# on linux & mac statically link libstdc++
unix: QMAKE_LFLAGS += -static-libstdc++

# dune won't compile without c++17 features only available in osx >= 10.14
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14

# for linux debug build, remove optimizations, add coverage & ASAN
CONFIG(debug, debug|release) {
    unix: QMAKE_CXXFLAGS_RELEASE -= -O2
    unix: QMAKE_CXXFLAGS += --coverage -fsanitize=address -fno-omit-frame-pointer
    unix: QMAKE_LFLAGS += --coverage -fsanitize=address -fno-omit-frame-pointer
}

# on windows add flags to support large object files
# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
win32: QMAKE_CXXFLAGS += -Wa,-mbig-obj

# on osx: set visibility to match setting used for compiling static Qt5 libs
mac: QMAKE_CXXFLAGS += -fvisibility=hidden

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$$[QT_INSTALL_HEADERS]/qt5" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtCore" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtWidgets" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtGui" \
    -isystem "$$[QT_INSTALL_HEADERS]/QtTest" \

