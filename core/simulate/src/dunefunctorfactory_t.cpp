// todo: re-enable tiff tests once
// https://gitlab.dune-project.org/copasi/dune-copasi/-/issues/80 is resolved

#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunefunctorfactory.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/model.hpp"
#include <QDir>
#include <QFile>
#include <cmath>
#include <locale>

using namespace sme;
using namespace sme::test;

static Dune::ParameterTree getConfig(const simulate::DuneConverter &dc) {
  Dune::ParameterTree config;
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);
  return config;
}

TEST_CASE("DUNE: SmeFunctorFactory",
          "[core/simulate/dunefunctorfactory][core/"
          "simulate][core][dunefunctorfactory][dune]") {
  // create a SmeFunctorFactory for 2D
  // simulate::SmeFunctorFactory<2> factory2d;
  //
  // // create a SmeFunctorFactory for 3D
  // simulate::SmeFunctorFactory<3> factory3d;
}
