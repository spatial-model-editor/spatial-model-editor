TARGET = gui
TEMPLATE = lib

QT += core gui widgets printsupport
CONFIG += static

include(../../topdir.pri)
include($$TOPDIR/common.pri)
include($$TOPDIR/ext/ext.pri)
include($$TOPDIR/src/core/core.pri)

SOURCES += \
    $$TOPDIR/src/gui/mainwindow.cpp \
    $$TOPDIR/src/gui/guiutils.cpp \
    $$TOPDIR/src/gui/dialogs/dialogabout.cpp \
    $$TOPDIR/src/gui/dialogs/dialoganalytic.cpp \
    $$TOPDIR/src/gui/dialogs/dialogconcentrationimage.cpp \
    $$TOPDIR/src/gui/dialogs/dialogimagesize.cpp \
    $$TOPDIR/src/gui/dialogs/dialogunits.cpp \
    $$TOPDIR/src/gui/tabs/qtabfunctions.cpp \
    $$TOPDIR/src/gui/tabs/qtabgeometry.cpp \
    $$TOPDIR/src/gui/tabs/qtabreactions.cpp \
    $$TOPDIR/src/gui/tabs/qtabsimulate.cpp \
    $$TOPDIR/src/gui/tabs/qtabspecies.cpp \
    $$TOPDIR/src/gui/widgets/qlabelmousetracker.cpp \
    $$TOPDIR/src/gui/widgets/qplaintextmathedit.cpp \

HEADERS += \
    $$TOPDIR/src/gui/mainwindow.hpp \
    $$TOPDIR/src/gui/guiutils.hpp \
    $$TOPDIR/src/gui/dialogs/dialogabout.hpp \
    $$TOPDIR/src/gui/dialogs/dialoganalytic.hpp \
    $$TOPDIR/src/gui/dialogs/dialogconcentrationimage.hpp \
    $$TOPDIR/src/gui/dialogs/dialogimagesize.hpp \
    $$TOPDIR/src/gui/dialogs/dialogunits.hpp \
    $$TOPDIR/src/gui/tabs/qtabfunctions.hpp \
    $$TOPDIR/src/gui/tabs/qtabgeometry.hpp \
    $$TOPDIR/src/gui/tabs/qtabreactions.hpp \
    $$TOPDIR/src/gui/tabs/qtabsimulate.hpp \
    $$TOPDIR/src/gui/tabs/qtabspecies.hpp \
    $$TOPDIR/src/gui/widgets/qlabelmousetracker.hpp \
    $$TOPDIR/src/gui/widgets/qplaintextmathedit.hpp \

FORMS += \
    $$TOPDIR/src/gui/mainwindow.ui \
    $$TOPDIR/src/gui/dialogs/dialogabout.ui \
    $$TOPDIR/src/gui/dialogs/dialoganalytic.ui \
    $$TOPDIR/src/gui/dialogs/dialogconcentrationimage.ui \
    $$TOPDIR/src/gui/dialogs/dialogimagesize.ui \
    $$TOPDIR/src/gui/dialogs/dialogunits.ui \
    $$TOPDIR/src/gui/tabs/qtabfunctions.ui \
    $$TOPDIR/src/gui/tabs/qtabgeometry.ui \
    $$TOPDIR/src/gui/tabs/qtabreactions.ui \
    $$TOPDIR/src/gui/tabs/qtabsimulate.ui \
    $$TOPDIR/src/gui/tabs/qtabspecies.ui \

INCLUDEPATH += \
    $$TOPDIR/src/gui \
    $$TOPDIR/src/gui/dialogs \
    $$TOPDIR/src/gui/tabs \
    $$TOPDIR/src/gui/widgets \
