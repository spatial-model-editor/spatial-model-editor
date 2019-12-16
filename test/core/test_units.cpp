#include "catch_wrapper.hpp"
#include "units.hpp"

SCENARIO("Units", "[core][units]") {
  auto timeUnits = units::UnitVector{{{"hour", "h", "second", 0, 1, 3600},
                                      {"minute", "m", "second", 0, 1, 60},
                                      {"second", "s", "second", 0},
                                      {"millisecond", "ms", "second", -3},
                                      {"microsecond", "us", "second", -6}}};
  auto lengthUnits = units::UnitVector{{{"metre", "m", "metre", 0},
                                        {"decimetre", "dm", "metre", -1},
                                        {"centimetre", "cm", "metre", -2},
                                        {"millimetre", "mm", "metre", -3},
                                        {"micrometre", "um", "metre", -6},
                                        {"nanometre", "nm", "metre", -9}}};
  auto volumeUnits =
      units::UnitVector{{{"litre", "L", "litre", 0},
                         {"decilitre", "dL", "litre", -1},
                         {"centilitre", "cL", "litre", -2},
                         {"millilitre", "mL", "litre", -3},
                         {"cubic metre", "m^3", "metre", 0, 3},
                         {"cubic decimetre", "dm^3", "metre", -1, 3},
                         {"cubic centimetre", "cm^3", "metre", -2, 3},
                         {"cubic millimetre", "mm^3", "metre", -3, 3}}};
  auto amountUnits = units::UnitVector{
      {{"mole", "mol", "mole", 0}, {"millimole", "mmol", "mole", -3}}};
  auto modelUnits =
      units::ModelUnits(timeUnits, lengthUnits, volumeUnits, amountUnits);
  WHEN("no index is set: zero used by default") {
    auto units = std::vector<const units::Unit*>{
        &(modelUnits.getTime()), &(modelUnits.getLength()),
        &(modelUnits.getVolume()), &(modelUnits.getAmount())};
    auto refs = std::vector<const units::Unit*>{
        &(timeUnits.getUnits()[0]), &(lengthUnits.getUnits()[0]),
        &(volumeUnits.getUnits()[0]), &(amountUnits.getUnits()[0])};
    for (std::size_t i = 0; i < units.size(); ++i) {
      const auto& unit = units[i];
      const auto& ref = refs[i];
      REQUIRE(unit->name == ref->name);
      REQUIRE(unit->symbol == ref->symbol);
      REQUIRE(unit->kind == ref->kind);
      REQUIRE(unit->scale == ref->scale);
      REQUIRE(unit->exponent == ref->exponent);
      REQUIRE(unit->multiplier == dbl_approx(ref->multiplier));
    }
  }
  WHEN("index is set: units updated") {
    modelUnits.setTime(2);
    REQUIRE(modelUnits.getTimeIndex() == 2);
    REQUIRE(modelUnits.getTime().name == timeUnits.getUnits()[2].name);
    modelUnits.setLength(4);
    REQUIRE(modelUnits.getLengthIndex() == 4);
    REQUIRE(modelUnits.getLength().name == lengthUnits.getUnits()[4].name);
    modelUnits.setVolume(3);
    REQUIRE(modelUnits.getVolumeIndex() == 3);
    REQUIRE(modelUnits.getVolume().name == volumeUnits.getUnits()[3].name);
    modelUnits.setAmount(1);
    REQUIRE(modelUnits.getAmountIndex() == 1);
    REQUIRE(modelUnits.getAmount().name == amountUnits.getUnits()[1].name);
  }
  WHEN("no index is set: derived units correct") {
    REQUIRE(modelUnits.getLength().symbol.toStdString() == "m");
    REQUIRE(modelUnits.getTime().symbol.toStdString() == "h");
    REQUIRE(modelUnits.getDiffusion().toStdString() == "m^2/h");

    REQUIRE(modelUnits.getAmount().symbol.toStdString() == "mol");
    REQUIRE(modelUnits.getVolume().symbol.toStdString() == "L");
    REQUIRE(modelUnits.getConcentration().toStdString() == "mol/L");
  }
  WHEN("index is set: derived units updated") {
    modelUnits.setTime(2);
    modelUnits.setLength(2);
    REQUIRE(modelUnits.getLength().symbol.toStdString() == "cm");
    REQUIRE(modelUnits.getTime().symbol.toStdString() == "s");
    REQUIRE(modelUnits.getDiffusion().toStdString() == "cm^2/s");

    modelUnits.setAmount(1);
    modelUnits.setVolume(4);
    REQUIRE(modelUnits.getAmount().symbol.toStdString() == "mmol");
    REQUIRE(modelUnits.getVolume().symbol.toStdString() == "m^3");
    REQUIRE(modelUnits.getConcentration().toStdString() == "mmol/m^3");
  }
}
