QT += testlib

TARGET = test
TEMPLATE = app

include(../topdir.pri)
include(../src/core/core.pri)
include(../src/gui/gui.pri)
include(../common.pri)
LIBS += -L../src/gui -lgui -L../src/core -lcore
include(../ext/ext.pri)

SOURCES += \
    main.cpp \
    test_utils/catch_wrapper.cpp \
    test_utils/qt_test_utils.cpp \
    gui/test_mainwindow.cpp \
    gui/dialogs/test_dialoganalytic.cpp \
    gui/dialogs/test_dialogconcentrationimage.cpp \
    gui/dialogs/test_dialogdisplayoptions.cpp \
    gui/dialogs/test_dialogimagesize.cpp \
    gui/dialogs/test_dialogimageslice.cpp \
    gui/dialogs/test_dialogintegratoroptions.cpp \
    gui/dialogs/test_dialogunits.cpp \
    gui/widgets/test_qlabelmousetracker.cpp \
    gui/widgets/test_qplaintextmathedit.cpp \
    gui/tabs/test_tabfunctions.cpp \
    gui/tabs/test_tabgeometry.cpp \
    gui/tabs/test_tabreactions.cpp \
    gui/tabs/test_tabsimulate.cpp \
    gui/tabs/test_tabspecies.cpp \
    core/test_dune.cpp \
    core/test_geometry.cpp \
    core/test_mesh.cpp \
    core/test_pde.cpp \
    core/test_sbml.cpp \
    core/test_simulate.cpp \
    core/test_symbolic.cpp \
    core/test_units.cpp \
    core/test_utils.cpp \

HEADERS += \
    test_utils/catch_wrapper.hpp \
    test_utils/qt_test_utils.hpp \
    test_utils/sbml_test_data/ABtoC.hpp \
    test_utils/sbml_test_data/very_simple_model.hpp \
    test_utils/sbml_test_data/yeast_glycolysis.hpp \
    test_utils/sbml_test_data/invalid_dune_names.hpp \

INCLUDEPATH += test_utils $${TOPDIR}/ext/catch
