QT += testlib

TARGET = test
TEMPLATE = app

include(common.pri)

# enable (at compile time) TRACE and DEBUG spdlog logging
DEFINES += SPDLOG_ACTIVE_LEVEL="SPDLOG_LEVEL_TRACE"

SOURCES += \
    test/src/main.cpp \
    test/src/test_dialogdimensions.cpp \
    test/src/test_dune.cpp \
    test/src/test_geometry.cpp \
    test/src/test_mainwindow.cpp \
    test/src/test_mesh.cpp \
    test/src/test_numerics.cpp \
    test/src/test_ostream_overloads.cpp \
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

INCLUDEPATH += test/inc

# for linux debug build, remove optimizations, add coverage & ASAN
CONFIG(debug, debug|release) {
    unix: QMAKE_CXXFLAGS_RELEASE -= -O2
    unix: QMAKE_CXXFLAGS += --coverage -fsanitize=address -fno-omit-frame-pointer
    unix: QMAKE_LFLAGS += --coverage -fsanitize=address -fno-omit-frame-pointer
}
