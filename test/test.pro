QT += testlib

TARGET = test
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
    src/main.cpp \
    src/test_dialoganalytic.cpp \
    src/test_dialogimagesize.cpp \
    src/test_dialogunits.cpp \
    src/test_dune.cpp \
    src/test_geometry.cpp \
    src/test_mainwindow.cpp \
    src/test_mesh.cpp \
    src/test_numerics.cpp \
    src/test_pde.cpp \
    src/test_qlabelmousetracker.cpp \
    src/test_sbml.cpp \
    src/test_simulate.cpp \
    src/test_symbolic.cpp \
    src/test_units.cpp \
    src/test_utils.cpp \

HEADERS += \
    inc/catch_wrapper.hpp \
    inc/qt_test_utils.hpp \
    inc/sbml_test_data/ABtoC.hpp \
    inc/sbml_test_data/very_simple_model.hpp \
    inc/sbml_test_data/yeast_glycolysis.hpp \
    inc/sbml_test_data/invalid_dune_names.hpp \

INCLUDEPATH += inc $${TOPDIR}/ext/catch
