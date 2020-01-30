TARGET = core
TEMPLATE = lib

QT += core gui
CONFIG += static

include(../../topdir.pri)
include($$TOPDIR/common.pri)
include($$TOPDIR/ext/ext.pri)

SOURCES += \
    $$TOPDIR/src/core/boundary.cpp \
    $$TOPDIR/src/core/duneini.cpp \
    $$TOPDIR/src/core/dunesim.cpp \
    $$TOPDIR/src/core/geometry.cpp \
    $$TOPDIR/src/core/mesh.cpp \
    $$TOPDIR/src/core/pde.cpp \
    $$TOPDIR/src/core/pixelsim.cpp \
    $$TOPDIR/src/core/sbml.cpp \
    $$TOPDIR/src/core/simulate.cpp \
    $$TOPDIR/src/core/symbolic.cpp \
    $$TOPDIR/src/core/tiff.cpp \
    $$TOPDIR/src/core/triangulate.cpp \
    $$TOPDIR/src/core/units.cpp \
    $$TOPDIR/src/core/utils.cpp \
    $$TOPDIR/src/core/version.cpp \

HEADERS += \
    $$TOPDIR/src/core/basesim.hpp \
    $$TOPDIR/src/core/boundary.hpp \
    $$TOPDIR/src/core/duneini.hpp \
    $$TOPDIR/src/core/dunesim.hpp \
    $$TOPDIR/src/core/geometry.hpp \
    $$TOPDIR/src/core/logger.hpp \
    $$TOPDIR/src/core/mesh.hpp \
    $$TOPDIR/src/core/pde.hpp \
    $$TOPDIR/src/core/pixelsim.hpp \
    $$TOPDIR/src/core/sbml.hpp \
    $$TOPDIR/src/core/simulate.hpp \
    $$TOPDIR/src/core/symbolic.hpp \
    $$TOPDIR/src/core/tiff.hpp \
    $$TOPDIR/src/core/triangulate.hpp \
    $$TOPDIR/src/core/units.hpp \
    $$TOPDIR/src/core/utils.hpp \
    $$TOPDIR/src/core/version.hpp \

RESOURCES += \
    $$TOPDIR/src/core/resources/resources.qrc \

INCLUDEPATH += \
    $$TOPDIR/src/core \
