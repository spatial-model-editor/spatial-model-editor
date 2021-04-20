#include "catch_wrapper.hpp"
#include "duneconverter.hpp"
#include "math_test_utils.hpp"
#include "model.hpp"
#include "sbml_test_data/invalid_dune_names.hpp"
#include <QFile>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLWriter.h>

using namespace sme;

SCENARIO("DUNE: DuneConverter",
         "[core/simulate/duneconverter][core/simulate][core][duneconverter]") {
  GIVEN("ABtoC model") {

    // REQUIRE(symEq(std::string("0.1*0.1"), "0.01"));
    model::Model s;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());
    simulate::DuneConverter dc(s, true, {}, 14);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    REQUIRE(*line++ == "[grid]");
    REQUIRE(*line++ == "file = grid.msh");
    REQUIRE(*line++ == "initial_level = 0");
    REQUIRE(*line++ == "dimension = 2");
    while (line != ini.cend() && *line != "[model.compartments]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.compartments]");
    REQUIRE(*line++ == "comp = 0");
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.comp.initial]");
    REQUIRE(*line++ == "A = 1000*A_initialConcentration(x,y)");
    REQUIRE(*line++ == "B = 1000*B_initialConcentration(x,y)");
    REQUIRE(*line++ == "C = 0");
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.data]");
    REQUIRE(*line++ == "A_initialConcentration = A_initialConcentration.tif");
    REQUIRE(*line++ == "B_initialConcentration = B_initialConcentration.tif");
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.comp.reaction]");
    REQUIRE(symEq(*line++, "A = -0.0001*A*B"));
    REQUIRE(symEq(*line++, "B = -0.0001*A*B"));
    REQUIRE(symEq(*line++, "C = 0.0001*A*B"));
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.comp.reaction.jacobian]");
    REQUIRE(symEq(*line++, "dA__dA = -0.0001*B"));
    REQUIRE(symEq(*line++, "dA__dB = -0.0001*A"));
    REQUIRE(symEq(*line++, "dA__dC = 0"));
    REQUIRE(symEq(*line++, "dB__dA = -0.0001*B"));
    REQUIRE(symEq(*line++, "dB__dB = -0.0001*A"));
    REQUIRE(symEq(*line++, "dB__dC = 0"));
    REQUIRE(symEq(*line++, "dC__dA = 0.0001*B"));
    REQUIRE(symEq(*line++, "dC__dB = 0.0001*A"));
    REQUIRE(symEq(*line++, "dC__dC = 0"));
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.comp.diffusion]");
    REQUIRE(*line++ == "A = 0.4");
    REQUIRE(*line++ == "B = 0.4");
    REQUIRE(*line++ == "C = 25");
  }
  GIVEN("brusselator model") {
    model::Model s;
    QFile f(":/models/brusselator-model.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());

    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    while (line != ini.cend() && *line != "[model.compartment.reaction]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.compartment.reaction]");
    // TODO: fix precision issue here: 1 != 0.9999999999999
    // REQUIRE(symEq(*line++, "X = 0.5 - 4.0*X + 1.0*X^2*Y"));
    // REQUIRE(symEq(*line++, "Y = 3.0*X - 1.0*X^2*Y"));
    // the rest of the species are constant,
    // so they don't exist from DUNE's point of view:
    // REQUIRE(*line++ == "");
  }
  GIVEN("very simple model") {
    model::Model s;
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());

    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    while (line != ini.cend() && *line != "[model.c2.reaction]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.c2.reaction]");
    REQUIRE(*line++ == "A_c2 = 0.0");
    REQUIRE(*line++ == "B_c2 = 0.0");
    while (line != ini.cend() && *line != "[model.c3.reaction]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.c3.reaction]");
    REQUIRE(symEq(*line++, "A_c3 = -0.3*A_c3"));
    REQUIRE(symEq(*line++, "B_c3 = 0.3*A_c3"));
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.c3.reaction.jacobian]");
    REQUIRE(symEq(*line++, "dA_c3__dA_c3 = -0.3"));
    REQUIRE(symEq(*line++, "dA_c3__dB_c3 = 0"));
    REQUIRE(symEq(*line++, "dB_c3__dA_c3 = 0.3"));
    REQUIRE(symEq(*line++, "dB_c3__dB_c3 = 0"));
  }
  GIVEN("species names that are invalid dune variables") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    while (line != ini.cend() && *line != "[model.comp.reaction]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.comp.reaction]");
    REQUIRE(symEq(*line++, "dim_ = -1e-7*x__*dim_"));
    REQUIRE(symEq(*line++, "x__ = -1e-7*x__*dim_"));
    REQUIRE(symEq(*line++, "x_ = 1e-7*x__*dim_"));
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.comp.reaction.jacobian]");
    REQUIRE(symEq(*line++, "ddim___ddim_ = -1e-7*x__"));
    REQUIRE(symEq(*line++, "ddim___dx__ = -1e-7*dim_"));
    REQUIRE(symEq(*line++, "ddim___dx_ = 0"));
  }
}
