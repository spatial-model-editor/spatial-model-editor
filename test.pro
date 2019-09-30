QT += testlib

TARGET = test
TEMPLATE = app

include(common.pri)

SOURCES += \
    test/src/main.cpp \
    test/src/test_dialogdimensions.cpp \
    test/src/test_dune.cpp \
    test/src/test_geometry.cpp \
    test/src/test_mainwindow.cpp \
    test/src/test_mesh.cpp \
    test/src/test_numerics.cpp \
    test/src/test_ostream_overloads.cpp \
    test/src/test_pde.cpp \
    test/src/test_qlabelmousetracker.cpp \
    test/src/test_reactions.cpp \
    test/src/test_sbml.cpp \
    test/src/test_simulate.cpp \
    test/src/test_symbolic.cpp \
    test/src/test_utils.cpp \

HEADERS += \
    test/inc/catch.hpp \
    test/inc/qt_test_utils.hpp \
    test/inc/sbml_test_data/ABtoC.hpp \
    test/inc/sbml_test_data/very_simple_model.hpp \
    test/inc/sbml_test_data/yeast_glycolysis.hpp \
    test/inc/sbml_test_data/invalid_dune_names.hpp \

INCLUDEPATH += test/inc
