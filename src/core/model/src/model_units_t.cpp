#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "model_units.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

TEST_CASE("Units", "[core/model/model_units][core/model][core][model][units]") {
  auto timeUnits = model::UnitVector{{{"hour", "second", 0, 1, 3600},
                                      {"min", "second", 0, 1, 60},
                                      {"s", "second", 0},
                                      {"ms", "second", -3},
                                      {"us", "second", -6}}};
  auto lengthUnits = model::UnitVector{{{"m", "metre", 0},
                                        {"dm", "metre", -1},
                                        {"cm", "metre", -2},
                                        {"mm", "metre", -3},
                                        {"um", "metre", -6},
                                        {"nm", "metre", -9}}};
  auto volumeUnits = model::UnitVector{{{"L", "litre", 0},
                                        {"dL", "litre", -1},
                                        {"cL", "litre", -2},
                                        {"mL", "litre", -3},
                                        {"m3", "metre", 0, 3},
                                        {"dm3", "metre", -1, 3},
                                        {"cm3", "metre", -2, 3},
                                        {"mm3", "metre", -3, 3}}};
  auto amountUnits =
      model::UnitVector{{{"mol", "mole", 0}, {"mmol", "mole", -3}}};
  SECTION("unit") {
    model::Unit u1{"hour", "second", 0, 1, 3600};
    model::Unit u2{"zzzz", "second", 0, 1, 3600};
    // units are equal if mathematically equivalent
    // (so name can be different)
    REQUIRE(u1 == u2);
  }
  SECTION("no model") {
    auto modelUnits = model::ModelUnits();
    auto units = std::vector<const model::Unit *>{
        &(modelUnits.getTime()), &(modelUnits.getLength()),
        &(modelUnits.getVolume()), &(modelUnits.getAmount())};
    auto refs = std::vector<const model::Unit *>{
        &(timeUnits.getUnits()[2]), &(lengthUnits.getUnits()[2]),
        &(volumeUnits.getUnits()[3]), &(amountUnits.getUnits()[1])};
    modelUnits.setHasUnsavedChanges(false);
    REQUIRE(modelUnits.getHasUnsavedChanges() == false);
    for (std::size_t i = 0; i < units.size(); ++i) {
      const auto &unit = units[i];
      const auto &ref = refs[i];
      REQUIRE(unit->name == ref->name);
      REQUIRE(unit->kind == ref->kind);
      REQUIRE(unit->scale == ref->scale);
      REQUIRE(unit->exponent == ref->exponent);
      REQUIRE(unit->multiplier == dbl_approx(ref->multiplier));
    }
    SECTION("default indices: derived units correct") {
      REQUIRE(modelUnits.getLength().name == "cm");
      REQUIRE(modelUnits.getTime().name == "s");
      REQUIRE(modelUnits.getDiffusion() == "cm^2/s");

      REQUIRE(modelUnits.getAmount().name == "mmol");
      REQUIRE(modelUnits.getVolume().name == "mL");
      REQUIRE(modelUnits.getConcentration() == "mmol/mL");
    }
    SECTION("index is set: units updated") {
      REQUIRE(modelUnits.getHasUnsavedChanges() == false);
      modelUnits.setTimeIndex(2);
      REQUIRE(modelUnits.getHasUnsavedChanges() == true);
      REQUIRE(modelUnits.getTimeIndex() == 2);
      REQUIRE(modelUnits.getTime().name == timeUnits.getUnits()[2].name);
      modelUnits.setLengthIndex(4);
      REQUIRE(modelUnits.getLengthIndex() == 4);
      REQUIRE(modelUnits.getLength().name == lengthUnits.getUnits()[4].name);
      modelUnits.setVolumeIndex(3);
      REQUIRE(modelUnits.getVolumeIndex() == 3);
      REQUIRE(modelUnits.getVolume().name == volumeUnits.getUnits()[3].name);
      modelUnits.setAmountIndex(1);
      REQUIRE(modelUnits.getAmountIndex() == 1);
      REQUIRE(modelUnits.getAmount().name == amountUnits.getUnits()[1].name);
    }
    SECTION("index is set: derived units updated") {
      REQUIRE(modelUnits.getHasUnsavedChanges() == false);
      modelUnits.setTimeIndex(2);
      modelUnits.setLengthIndex(2);
      REQUIRE(modelUnits.getHasUnsavedChanges() == true);
      REQUIRE(modelUnits.getLength().name == "cm");
      REQUIRE(modelUnits.getTime().name == "s");
      REQUIRE(modelUnits.getDiffusion() == "cm^2/s");

      modelUnits.setAmountIndex(1);
      modelUnits.setVolumeIndex(4);
      REQUIRE(modelUnits.getAmount().name == "mmol");
      REQUIRE(modelUnits.getVolume().name == "m3");
      REQUIRE(modelUnits.getConcentration() == "mmol/m3");
    }
  }
  SECTION("model with custom units") {
    auto doc{test::getTestSbmlDoc("weird-units")};
    auto modelUnits = model::ModelUnits(doc->getModel());
    modelUnits.setHasUnsavedChanges(false);
    REQUIRE(modelUnits.getHasUnsavedChanges() == false);
    REQUIRE(modelUnits.getTime().name == "megasecond");
    REQUIRE(modelUnits.getTime().kind == "second");
    REQUIRE(modelUnits.getTime().exponent == 1);
    REQUIRE(modelUnits.getTime().scale == 6);
    REQUIRE(modelUnits.getTime().multiplier == dbl_approx(1.0));
    REQUIRE(modelUnits.getHasUnsavedChanges() == false);
  }
}
