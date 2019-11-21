QT += testlib

TARGET = test_unity
TEMPLATE = app

include(../topdir.pri)
include(../src/src.pri)
include(../common.pri)
win32 {
    CONFIG(release, debug|release):LIBS += -L../src/release -lmainwindow
    CONFIG(debug, debug|release):LIBS += -L../src/debug -lmainwindow
}
!win32: LIBS += -L../src -lmainwindow
include(../ext/ext.pri)

SOURCES += \
    main_unity.cpp

HEADERS += \
    $${TOPDIR}/test/inc/qt_test_utils.hpp

INCLUDEPATH += inc $${TOPDIR}/ext/catch $${TOPDIR}/test/inc $${TOPDIR}/test/src
