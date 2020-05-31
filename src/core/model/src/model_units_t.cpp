#include "catch_wrapper.hpp"
#include "model_units.hpp"

SCENARIO("Units", "[core/model/model_units][core/model][core][model][units]") {
  auto timeUnits = model::UnitVector{{{"hour", "h", "second", 0, 1, 3600},
                                      {"minute", "m", "second", 0, 1, 60},
                                      {"second", "s", "second", 0},
                                      {"millisecond", "ms", "second", -3},
                                      {"microsecond", "us", "second", -6}}};
  auto lengthUnits = model::UnitVector{{{"metre", "m", "metre", 0},
                                        {"decimetre", "dm", "metre", -1},
                                        {"centimetre", "cm", "metre", -2},
                                        {"millimetre", "mm", "metre", -3},
                                        {"micrometre", "um", "metre", -6},
                                        {"nanometre", "nm", "metre", -9}}};
  auto volumeUnits =
      model::UnitVector{{{"litre", "L", "litre", 0},
                         {"decilitre", "dL", "litre", -1},
                         {"centilitre", "cL", "litre", -2},
                         {"millilitre", "mL", "litre", -3},
                         {"cubic metre", "m^3", "metre", 0, 3},
                         {"cubic decimetre", "dm^3", "metre", -1, 3},
                         {"cubic centimetre", "cm^3", "metre", -2, 3},
                         {"cubic millimetre", "mm^3", "metre", -3, 3}}};
  auto amountUnits = model::UnitVector{
      {{"mole", "mol", "mole", 0}, {"millimole", "mmol", "mole", -3}}};
  auto modelUnits = model::ModelUnits();
  WHEN("default indices") {
    auto units = std::vector<const model::Unit *>{
        &(modelUnits.getTime()), &(modelUnits.getLength()),
        &(modelUnits.getVolume()), &(modelUnits.getAmount())};
    auto refs = std::vector<const model::Unit *>{
        &(timeUnits.getUnits()[2]), &(lengthUnits.getUnits()[2]),
        &(volumeUnits.getUnits()[3]), &(amountUnits.getUnits()[1])};
    for (std::size_t i = 0; i < units.size(); ++i) {
      const auto &unit = units[i];
      const auto &ref = refs[i];
      REQUIRE(unit->name == ref->name);
      REQUIRE(unit->symbol == ref->symbol);
      REQUIRE(unit->kind == ref->kind);
      REQUIRE(unit->scale == ref->scale);
      REQUIRE(unit->exponent == ref->exponent);
      REQUIRE(unit->multiplier == dbl_approx(ref->multiplier));
    }
  }
  WHEN("default indices: derived units correct") {
    REQUIRE(modelUnits.getLength().symbol.toStdString() == "cm");
    REQUIRE(modelUnits.getTime().symbol.toStdString() == "s");
    REQUIRE(modelUnits.getDiffusion().toStdString() == "cm^2/s");

    REQUIRE(modelUnits.getAmount().symbol.toStdString() == "mmol");
    REQUIRE(modelUnits.getVolume().symbol.toStdString() == "mL");
    REQUIRE(modelUnits.getConcentration().toStdString() == "mmol/mL");
  }
  WHEN("index is set: units updated") {
    modelUnits.setTimeIndex(2);
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
  WHEN("index is set: derived units updated") {
    modelUnits.setTimeIndex(2);
    modelUnits.setLengthIndex(2);
    REQUIRE(modelUnits.getLength().symbol.toStdString() == "cm");
    REQUIRE(modelUnits.getTime().symbol.toStdString() == "s");
    REQUIRE(modelUnits.getDiffusion().toStdString() == "cm^2/s");

    modelUnits.setAmountIndex(1);
    modelUnits.setVolumeIndex(4);
    REQUIRE(modelUnits.getAmount().symbol.toStdString() == "mmol");
    REQUIRE(modelUnits.getVolume().symbol.toStdString() == "m^3");
    REQUIRE(modelUnits.getConcentration().toStdString() == "mmol/m^3");
  }
}
