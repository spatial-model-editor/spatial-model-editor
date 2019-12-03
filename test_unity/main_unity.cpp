// unity (single compilation unit) build of tests
// to reduce compilation time on CI builds

#define CATCH_CONFIG_RUNNER

#include "main.cpp"
#include "test_dialoganalytic.cpp"
#include "test_dialogconcentrationimage.cpp"
#include "test_dialogimagesize.cpp"
#include "test_dialogunits.cpp"
#include "test_dune.cpp"
#include "test_geometry.cpp"
#include "test_mainwindow.cpp"
#include "test_mesh.cpp"
#include "test_pde.cpp"
#include "test_qlabelmousetracker.cpp"
#include "test_qplaintextmathedit.cpp"
#include "test_sbml.cpp"
#include "test_simulate.cpp"
#include "test_symbolic.cpp"
#include "test_units.cpp"
#include "test_utils.cpp"
