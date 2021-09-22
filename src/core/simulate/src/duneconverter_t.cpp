#include "catch_wrapper.hpp"
#include "duneconverter.hpp"
#include "math_test_utils.hpp"
#include "model.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("DUNE: DuneConverter",
          "[core/simulate/duneconverter][core/simulate][core][duneconverter]") {
  SECTION("ABtoC model") {
    auto s{getExampleModel(Mod::ABtoC)};
    simulate::DuneConverter dc(s, {}, true, {}, 14);
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
  SECTION("brusselator model") {
    auto s{getExampleModel(Mod::Brusselator)};
    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    while (line != ini.cend() && *line != "[model.compartment.reaction]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.compartment.reaction]");
    REQUIRE(symEq(*line++, "X = 0.5 - 4.0*X + 2.0*X^2*Y"));
    REQUIRE(symEq(*line++, "Y = 3.0*X - 2.0*X^2*Y"));
    //     the rest of the species are constant,
    //     so they don't exist from DUNE's point of view:
    REQUIRE(*line++ == "");
  }
  SECTION("very simple model") {
    auto s{getExampleModel(Mod::VerySimpleModel)};
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
  SECTION("species names that are invalid dune variables") {
    auto s{getTestModel("invalid-dune-names")};
    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    while (line != ini.cend() && *line != "[model.comp.reaction]") {
      ++line;
    }
    REQUIRE(*line++ == "[model.comp.reaction]");
    REQUIRE(symEq(*line++, "dim_ = -0.1*x__*dim_"));
    REQUIRE(symEq(*line++, "x__ = -0.1*x__*dim_"));
    REQUIRE(symEq(*line++, "x_ = 0.1*x__*dim_"));
    REQUIRE(*line++ == "");
    REQUIRE(*line++ == "[model.comp.reaction.jacobian]");
    REQUIRE(symEq(*line++, "ddim___ddim_ = -0.1*x__"));
    REQUIRE(symEq(*line++, "ddim___dx__ = -0.1*dim_"));
    REQUIRE(symEq(*line++, "ddim___dx_ = 0"));
  }
  SECTION("brusselator model with substitutions") {
    auto s{getExampleModel(Mod::Brusselator)};
    // X = k_1/2 - (3+k_1)*X + k_2 X^2 Y
    // Y = 3 X - k_2 X^2 Y
    {
      simulate::DuneConverter dc(s);
      QStringList ini = dc.getIniFile().split("\n");
      auto line = ini.cbegin();
      while (line != ini.cend() && *line != "[model.compartment.reaction]") {
        ++line;
      }
      // default values: k1 = 1, k2 = 2
      REQUIRE(*line++ == "[model.compartment.reaction]");
      REQUIRE(symEq(*line++, "X = 0.5 - 4.0*X + 2.0*X^2*Y"));
      REQUIRE(symEq(*line++, "Y = 3.0*X - 2.0*X^2*Y"));
    }
    {
      simulate::DuneConverter dc(s, {{"k1", 1.74}, {"k2", 5.2}});
      QStringList ini = dc.getIniFile().split("\n");
      auto line = ini.cbegin();
      while (line != ini.cend() && *line != "[model.compartment.reaction]") {
        ++line;
      }
      REQUIRE(*line++ == "[model.compartment.reaction]");
      REQUIRE(symEq(*line++, "X = 0.87 - 4.74*X + 5.2*X^2*Y"));
      REQUIRE(symEq(*line++, "Y = 3.0*X - 5.2*X^2*Y"));
    }
    {
      simulate::DuneConverter dc(
          s, {{"k1", -1.74}, {"k2", -20.2}, {"doesnotexist", -1}});
      QStringList ini = dc.getIniFile().split("\n");
      auto line = ini.cbegin();
      while (line != ini.cend() && *line != "[model.compartment.reaction]") {
        ++line;
      }
      REQUIRE(*line++ == "[model.compartment.reaction]");
      REQUIRE(symEq(*line++, "X = -0.87 - 1.26*X - 20.2*X^2*Y"));
      REQUIRE(symEq(*line++, "Y = 3.0*X + 20.2*X^2*Y"));
    }
  }
}
