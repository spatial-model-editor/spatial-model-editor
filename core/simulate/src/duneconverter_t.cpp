#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

static QStringList::const_iterator find_line(const QString &line,
                                             const QStringList &lines) {
  auto iter{lines.cbegin()};
  while (iter != lines.cend() && *iter != line) {
    ++iter;
  }
  return iter;
}

TEST_CASE("DUNE: DuneConverter",
          "[core/simulate/duneconverter][core/simulate][core][duneconverter]") {
  SECTION("ABtoC model") {
    auto s{getExampleModel(Mod::ABtoC)};
    simulate::DuneConverter dc(s, {}, true, {}, 14);
    QStringList ini = dc.getIniFile().split("\n");
    auto line = ini.cbegin();
    REQUIRE(*line++ == "[grid]");
    REQUIRE(*line++ == "path = dune.msh");
    REQUIRE(*line++ == "dimension = 2");
    line = find_line("[compartments]", ini);
    REQUIRE(*line++ == "[compartments]");
    REQUIRE(*line++ == "comp.expression = (gmsh_id == 1)");
    REQUIRE(*line++ == "");
    line = find_line("[model.scalar_field.C]", ini);
    REQUIRE(*line++ == "[model.scalar_field.C]");
    REQUIRE(*line++ == "compartment = comp");
    REQUIRE(*line++ == "initial.expression = 0");
    REQUIRE(*line++ == "storage.expression = 1");
    REQUIRE(symEq(*line++, "reaction.expression = 0.0001*A*B"));
    REQUIRE(symEq(*line++, "reaction.jacobian.A.expression = 0.0001*B"));
    REQUIRE(symEq(*line++, "reaction.jacobian.B.expression = 0.0001*A"));
    REQUIRE(symEq(*line++, "reaction.jacobian.C.expression = 0"));
    REQUIRE(symEq(*line++, "cross_diffusion.C.expression = 25"));
  }
  SECTION("brusselator model") {
    auto s{getExampleModel(Mod::Brusselator)};
    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line{find_line("[model.scalar_field.X]", ini)};
    REQUIRE(*line++ == "[model.scalar_field.X]");
    REQUIRE(*line++ == "compartment = compartment");
    REQUIRE(*line++ == "initial.expression = 0");
    REQUIRE(*line++ == "storage.expression = 1");
    REQUIRE(symEq(*line++, "reaction.expression = 0.5 - 4.0*X + 2.0*X^2*Y"));
    REQUIRE(symEq(*line++, "reaction.jacobian.X.expression = -4.0 + 4.0*X*Y"));
    REQUIRE(symEq(*line++, "reaction.jacobian.Y.expression = 2.0*X^2"));
    REQUIRE(
        symEq(*line++, "cross_diffusion.X.expression = 0.200000000000000011"));
  }
  SECTION("very simple model") {
    auto s{getExampleModel(Mod::VerySimpleModel)};
    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line{find_line("[model.scalar_field.B_c2]", ini)};
    REQUIRE(*line++ == "[model.scalar_field.B_c2]");
    REQUIRE(*line++ == "compartment = c2");
    REQUIRE(*line++ == "initial.expression = 0");
    REQUIRE(*line++ == "storage.expression = 1");
    REQUIRE(symEq(*line++, "reaction.expression = 0.0"));
    REQUIRE(symEq(*line++, "reaction.jacobian.A_c2.expression = 0"));
    REQUIRE(symEq(*line++, "reaction.jacobian.B_c2.expression = 0"));
    REQUIRE(symEq(*line++, "cross_diffusion.B_c2.expression = 6"));
    REQUIRE(symEq(*line++, "outflow.c1.expression = 0.0002*B_c2"));
    REQUIRE(symEq(*line++, "outflow.c1.jacobian.A_c2.expression = 0"));
    REQUIRE(symEq(*line++, "outflow.c1.jacobian.B_c2.expression = 0.0002"));
    REQUIRE(symEq(*line++, "outflow.c1.jacobian.B_c1.expression = 0"));
    REQUIRE(symEq(*line++, "outflow.c3.expression = -0.0001*B_c3"));
    REQUIRE(symEq(*line++, "outflow.c3.jacobian.A_c2.expression = 0"));
    REQUIRE(symEq(*line++, "outflow.c3.jacobian.B_c2.expression = 0"));
    REQUIRE(symEq(*line++, "outflow.c3.jacobian.A_c3.expression = 0"));
    REQUIRE(symEq(*line++, "outflow.c3.jacobian.B_c3.expression = -0.0001"));
  }
  SECTION("species names that are invalid dune variables") {
    auto s{getTestModel("invalid-dune-names")};
    simulate::DuneConverter dc(s);
    QStringList ini = dc.getIniFile().split("\n");
    auto line{find_line("[model.scalar_field.position_x_]", ini)};
    REQUIRE(*line++ == "[model.scalar_field.position_x_]");
    REQUIRE(*line++ == "compartment = comp");
    REQUIRE(*line++ == "initial.expression = 0");
    REQUIRE(*line++ == "storage.expression = 1");
    REQUIRE(symEq(*line++, "reaction.expression = 0.1*dim_*position_x__"));
    REQUIRE(
        symEq(*line++, "reaction.jacobian.dim_.expression = 0.1*position_x__"));
    REQUIRE(
        symEq(*line++, "reaction.jacobian.position_x__.expression = 0.1*dim_"));
    REQUIRE(symEq(*line++, "reaction.jacobian.position_x_.expression = 0"));
  }
  SECTION("brusselator model with substitutions") {
    auto s{getExampleModel(Mod::Brusselator)};
    // X = k_1/2 - (3+k_1)*X + k_2 X^2 Y
    // Y = 3 X - k_2 X^2 Y
    {
      simulate::DuneConverter dc(s);
      QStringList ini = dc.getIniFile().split("\n");
      // default values: k1 = 1, k2 = 2
      auto line{find_line("[model.scalar_field.X]", ini)};
      line += 4;
      REQUIRE(symEq(*line++, "reaction.expression = 0.5 - 4.0*X + 2.0*X^2*Y"));
      line = find_line("[model.scalar_field.Y]", ini);
      line += 4;
      REQUIRE(symEq(*line++, "reaction.expression =  3.0*X - 2.0*X^2*Y"));
    }
    {
      simulate::DuneConverter dc(s, {{"k1", 1.74}, {"k2", 5.2}});
      QStringList ini = dc.getIniFile().split("\n");
      auto line{find_line("[model.scalar_field.X]", ini)};
      line += 4;
      REQUIRE(
          symEq(*line++, "reaction.expression = 0.87 - 4.74*X + 5.2*X^2*Y"));
      line = find_line("[model.scalar_field.Y]", ini);
      line += 4;
      REQUIRE(symEq(*line++, "reaction.expression = 3.0*X - 5.2*X^2*Y"));
    }
    {
      simulate::DuneConverter dc(
          s, {{"k1", -1.74}, {"k2", -20.2}, {"doesnotexist", -1}});
      QStringList ini = dc.getIniFile().split("\n");
      auto line{find_line("[model.scalar_field.X]", ini)};
      line += 4;
      REQUIRE(
          symEq(*line++, "reaction.expression = -0.87 - 1.26*X - 20.2*X^2*Y"));
      line = find_line("[model.scalar_field.Y]", ini);
      line += 4;
      REQUIRE(symEq(*line++, "reaction.expression = 3.0*X + 20.2*X^2*Y"));
    }
  }
}
