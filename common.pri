QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    src/boundary.cpp \
    src/dialogdimensions.cpp \
    src/dune.cpp \
    src/geometry.cpp \
    src/mainwindow.cpp \
    src/mesh.cpp \
    src/numerics.cpp \
    src/ostream_overloads.cpp \
    src/qlabelmousetracker.cpp \
    src/reactions.cpp \
    src/sbml.cpp \
    src/simulate.cpp \
    src/symbolic.cpp \
    src/triangulate.cpp \
    src/utils.cpp \
    src/version.cpp \

HEADERS += \
    inc/boundary.hpp \
    inc/dune.hpp \
    inc/dialogdimensions.hpp \
    inc/geometry.hpp \
    inc/exprtk_wrapper.hpp \
    inc/logger.hpp \
    inc/mainwindow.hpp \
    inc/mesh.hpp \
    inc/numerics.hpp \
    inc/ostream_overloads.hpp \
    inc/qlabelmousetracker.hpp \
    inc/reactions.hpp \
    inc/sbml.hpp \
    inc/simulate.hpp \
    inc/symbolic.hpp \
    inc/triangulate.hpp \
    inc/utils.hpp \
    inc/version.hpp \

FORMS += \
    ui/dialogdimensions.ui \
    ui/mainwindow.ui \

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += inc ext

include(ext/libs.pri)

# on linux, enable GCC compiler warnings
unix: QMAKE_CXXFLAGS += -Wall -Wcast-align -Wconversion -Wdouble-promotion -Wextra -Wformat=2 -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wshadow -Wsign-conversion -Wunused -Wpedantic

# on windows statically link libgcc & libstdc++ to avoid missing dll errors:
win32: QMAKE_LFLAGS += -static-libgcc -static-libstdc++

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
