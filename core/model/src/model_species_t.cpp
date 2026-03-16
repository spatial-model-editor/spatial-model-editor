#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <memory>

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML species",
          "[core/model/species][core/model][core][model][species]") {
  SECTION("Different initial concentration types") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};
    s.setHasUnsavedChanges(false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    REQUIRE(s.containsNonSpatialReactiveSpecies() == false);
    // initial species concentration
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Uniform);
    REQUIRE(s.getInitialConcentration("A_c1") == dbl_approx(1.0));
    // set analytic expression
    s.setAnalyticConcentration("A_c1", "exp(-2*x*x)");
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Analytic);
    REQUIRE(s.getInitialConcentration("A_c1") == dbl_approx(1.0));
    REQUIRE(symEq(s.getAnalyticConcentration("A_c1"), "exp(-2 * x * x)"));
  }
  SECTION("Non-spatial species") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};
    REQUIRE(s.containsNonSpatialReactiveSpecies() == false);
    // make B_c3 non-spatial
    s.setIsSpatial("B_c3", false);
    REQUIRE(s.containsNonSpatialReactiveSpecies() == true);
    // also make it constant
    s.setIsConstant("B_c3", true);
    REQUIRE(s.containsNonSpatialReactiveSpecies() == false);
    // constant spatial species are allowed
    REQUIRE(s.getIsSpatial("B_c2") == true);
    s.setIsConstant("B_c2", true);
    REQUIRE(s.getIsConstant("B_c2") == true);
    REQUIRE(s.getIsSpatial("B_c2") == true);
    REQUIRE(s.containsNonSpatialReactiveSpecies() == false);
    s.setIsConstant("B_c2", false);
    REQUIRE(s.getIsConstant("B_c2") == false);
    REQUIRE(s.getIsSpatial("B_c2") == true);
    REQUIRE(s.containsNonSpatialReactiveSpecies() == false);
  }
  SECTION("Constant spatial species preserve spatial initial conditions") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};
    const auto nVoxels{static_cast<std::size_t>(
        m.getGeometry().getImages().volume().nVoxels())};

    s.setIsConstant("A_c1", true);
    s.setIsSpatial("A_c1", true);
    REQUIRE(s.getIsConstant("A_c1") == true);
    REQUIRE(s.getIsSpatial("A_c1") == true);

    std::vector<double> sampledField(nVoxels, 0.0);
    const auto *comp = m.getCompartments().getCompartment("c1");
    std::size_t i{0};
    for (const auto &voxel : comp->getVoxels()) {
      const auto arrayIndex = common::voxelArrayIndex(
          m.getGeometry().getImages().volume(), voxel, true);
      sampledField[arrayIndex] = static_cast<double>(i++ % 7);
    }
    s.setSampledFieldConcentration("A_c1", sampledField);
    s.setIsConstant("A_c1", true);
    const auto storedSampledField{s.getSampledFieldConcentration("A_c1")};
    REQUIRE(s.getIsConstant("A_c1") == true);
    REQUIRE(s.getIsSpatial("A_c1") == true);
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Image);
    REQUIRE(storedSampledField != std::vector<double>(nVoxels, 0.0));

    model::Model mImageRoundTrip;
    mImageRoundTrip.importSBMLString(m.getXml().toStdString());
    REQUIRE(mImageRoundTrip.getSpecies().getIsConstant("A_c1") == true);
    REQUIRE(mImageRoundTrip.getSpecies().getIsSpatial("A_c1") == true);
    REQUIRE(mImageRoundTrip.getSpecies().getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Image);
    REQUIRE(mImageRoundTrip.getSpecies().getSampledFieldConcentration("A_c1") ==
            storedSampledField);

    s.setAnalyticConcentration("A_c1", "x");
    s.setIsConstant("A_c1", true);
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Analytic);
    REQUIRE(symEq(s.getAnalyticConcentration("A_c1"), "x"));

    model::Model mAnalyticRoundTrip;
    mAnalyticRoundTrip.importSBMLString(m.getXml().toStdString());
    REQUIRE(mAnalyticRoundTrip.getSpecies().getIsConstant("A_c1") == true);
    REQUIRE(mAnalyticRoundTrip.getSpecies().getIsSpatial("A_c1") == true);
    REQUIRE(mAnalyticRoundTrip.getSpecies().getInitialConcentrationType(
                "A_c1") == model::SpatialDataType::Analytic);
    REQUIRE(symEq(
        mAnalyticRoundTrip.getSpecies().getAnalyticConcentration("A_c1"), "x"));
  }
  SECTION(
      "Switching a species to non-spatial resets initial concentration type") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};

    s.setAnalyticConcentration("A_c1", "x");
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Analytic);
    s.setIsSpatial("A_c1", false);
    REQUIRE(s.getIsSpatial("A_c1") == false);
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Uniform);
    REQUIRE(s.getField("A_c1")->getIsUniformConcentration() == true);
    REQUIRE(s.getInitialConcentration("A_c1") == dbl_approx(1.0));

    s.setIsSpatial("A_c1", true);
    const auto nVoxels{static_cast<std::size_t>(
        m.getGeometry().getImages().volume().nVoxels())};
    std::vector<double> sampledField(nVoxels, 0.0);
    const auto *comp = m.getCompartments().getCompartment("c1");
    std::size_t i{0};
    for (const auto &voxel : comp->getVoxels()) {
      const auto arrayIndex = common::voxelArrayIndex(
          m.getGeometry().getImages().volume(), voxel, true);
      sampledField[arrayIndex] = static_cast<double>(i++ % 5);
    }
    s.setSampledFieldConcentration("A_c1", sampledField);
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Image);
    s.setIsSpatial("A_c1", false);
    REQUIRE(s.getIsSpatial("A_c1") == false);
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Uniform);
    REQUIRE(s.getField("A_c1")->getIsUniformConcentration() == true);
    REQUIRE(s.getInitialConcentration("A_c1") == dbl_approx(1.0));
  }
  SECTION("Remove species also removes dependents") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::Simulation sim(m);
    auto &s{m.getSpecies()};
    auto &r{m.getReactions()};
    r.setHasUnsavedChanges(false);
    s.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    // add minimal simulation data
    REQUIRE(m.getSimulationData().timePoints.size() == 1);
    sim.doTimesteps(0.001);
    REQUIRE(m.getSimulationData().timePoints.size() == 2);

    // initial species
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Uniform);
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 2);
    REQUIRE(s.getIds("c2")[0] == "A_c2");
    REQUIRE(s.getField("A_c2")->getId() == "A_c2");
    REQUIRE(s.getIds("c2")[1] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").size() == 2);
    REQUIRE(s.getIds("c3")[0] == "A_c3");
    REQUIRE(s.getField("A_c3")->getId() == "A_c3");
    REQUIRE(s.getIds("c3")[1] == "B_c3");
    REQUIRE(s.getField("B_c3")->getId() == "B_c3");

    // initial reactions
    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 1);
    REQUIRE(r.getName("A_B_conversion") == "A to B conversion");
    REQUIRE(r.getLocation("A_B_conversion") == "c3");
    REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.3));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
    REQUIRE(r.getLocation("A_uptake") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("A_uptake").size() == 1);
    REQUIRE(r.getParameterName("A_uptake", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
    REQUIRE(r.getLocation("A_transport") == "c2_c3_membrane");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getParameterName("A_transport", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.1));
    REQUIRE(r.getParameterName("A_transport", "k2") == "k2");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_transport") == "c2_c3_membrane");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);
    REQUIRE(r.getParameterName("B_transport", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.1));

    // make species B_c3 constant
    // this resets any existing simulation data
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.setIsConstant("B_c3", true);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(m.getSimulationData().timePoints.size() == 0);

    // make species B_c3 constant is no-op if already constant
    s.setHasUnsavedChanges(false);
    s.setIsConstant("B_c3", true);
    REQUIRE(s.getHasUnsavedChanges() == false);

    // make species constant is no-op if species doesn't exist
    s.setIsConstant("idontexist", false);
    s.setIsConstant("idontexist", true);
    REQUIRE(s.getHasUnsavedChanges() == false);

    // add minimal simulation data
    sim.doTimesteps(0.001);
    REQUIRE(m.getSimulationData().timePoints.size() == 2);
    // remove species B_c3
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.remove("B_c3");
    REQUIRE(r.getHasUnsavedChanges() == true);
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(m.getSimulationData().timePoints.size() == 0);
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 2);
    REQUIRE(s.getIds("c2")[0] == "A_c2");
    REQUIRE(s.getField("A_c2")->getId() == "A_c2");
    REQUIRE(s.getIds("c2")[1] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").size() == 1);
    REQUIRE(s.getIds("c3")[0] == "A_c3");
    REQUIRE(s.getField("A_c3")->getId() == "A_c3");
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
    REQUIRE(r.getLocation("A_uptake") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("A_uptake").size() == 1);
    REQUIRE(r.getParameterName("A_uptake", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").size() == 1);
    REQUIRE(r.getLocation("A_transport") == "c2_c3_membrane");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getParameterName("A_transport", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.1));
    REQUIRE(r.getParameterName("A_transport", "k2") == "k2");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));

    // remove species A_c2
    s.remove("A_c2");
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").size() == 1);
    REQUIRE(s.getIds("c3")[0] == "A_c3");
    REQUIRE(s.getField("A_c3")->getId() == "A_c3");
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").empty());
    REQUIRE(r.getLocation("A_transport") == "");
    REQUIRE(r.getParameterIds("A_transport").empty());
    REQUIRE(r.getParameterName("A_transport", "k1") == "");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
    REQUIRE(r.getParameterName("A_transport", "k2") == "");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));

    // remove species A_c3
    s.remove("A_c3");
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").empty());
    REQUIRE(r.getLocation("A_transport") == "");
    REQUIRE(r.getParameterIds("A_transport").empty());
    REQUIRE(r.getParameterName("A_transport", "k1") == "");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
    REQUIRE(r.getParameterName("A_transport", "k2") == "");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));

    // remove species A_c1
    s.remove("A_c1");
    REQUIRE(s.getIds("c1").size() == 1);
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getIds("c1")[0] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").empty());
    REQUIRE(r.getLocation("A_transport") == "");
    REQUIRE(r.getParameterIds("A_transport").empty());
    REQUIRE(r.getParameterName("A_transport", "k1") == "");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
    REQUIRE(r.getParameterName("A_transport", "k2") == "");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));

    // remove non-existent species: no-op
    r.setHasUnsavedChanges(false);
    s.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.remove("not_a_species");
    REQUIRE(s.getIds("c1").size() == 1);
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getIds("c1")[0] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);

    // remove species B_c1
    s.remove("B_c1");
    REQUIRE(r.getHasUnsavedChanges() == true);
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(s.getIds("c1").empty());
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getField("B_c1") == nullptr);
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_excretion") == "");
    REQUIRE(r.getParameterIds("B_excretion").empty());
    REQUIRE(r.getParameterName("B_excretion", "k1") == "");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c2_c3_membrane").empty());
    REQUIRE(r.getLocation("A_transport") == "");
    REQUIRE(r.getParameterIds("A_transport").empty());
    REQUIRE(r.getParameterName("A_transport", "k1") == "");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
    REQUIRE(r.getParameterName("A_transport", "k2") == "");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));

    // remove species B_c2
    s.remove("B_c2");
    REQUIRE(s.getIds("c1").empty());
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getField("B_c1") == nullptr);
    REQUIRE(s.getIds("c2").empty());
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getField("B_c2") == nullptr);
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_excretion") == "");
    REQUIRE(r.getParameterIds("B_excretion").empty());
    REQUIRE(r.getParameterName("B_excretion", "k1") == "");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c2_c3_membrane").empty());
    REQUIRE(r.getLocation("A_transport") == "");
    REQUIRE(r.getParameterIds("A_transport").empty());
    REQUIRE(r.getParameterName("A_transport", "k1") == "");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
    REQUIRE(r.getParameterName("A_transport", "k2") == "");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));
  }
  SECTION("Analytic conc that depends on modified parameter") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/776
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};
    // set analytic expression that depends on a parameter
    s.setAnalyticConcentration("A_c1", "param");
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Analytic);
    REQUIRE(symEq(s.getAnalyticConcentration("A_c1"), "param"));
    REQUIRE(common::average(s.getField("A_c1")->getConcentration()) ==
            dbl_approx(1.0));
    m.getParameters().setExpression("param", "2.0");
    REQUIRE(common::average(s.getField("A_c1")->getConcentration()) ==
            dbl_approx(2.0));
  }
  SECTION("Analytic conc involving piecewise") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};
    s.setAnalyticConcentration("A_c1", "piecewise(1,x<33,2,x>66,0)");
    REQUIRE(s.getInitialConcentrationType("A_c1") ==
            model::SpatialDataType::Analytic);
    REQUIRE(symEq(s.getAnalyticConcentration("A_c1"),
                  "piecewise(1, x < 33, 2, x > 66, 0)"));
    const auto &voxels{s.getField("A_c1")->getCompartment()->getVoxels()};
    const auto &concentration{s.getField("A_c1")->getConcentration()};
    REQUIRE(voxels.size() == concentration.size());
    auto correct_conc = [&m](const common::Voxel &voxel) {
      double x{m.getGeometry().getPhysicalPoint(voxel).p.x()};
      if (x < 33) {
        return 1;
      } else if (x > 66) {
        return 2;
      }
      return 0;
    };
    for (std::size_t i = 0; i < voxels.size(); ++i) {
      REQUIRE(concentration[i] == dbl_approx(correct_conc(voxels[i])));
    }
  }
  SECTION("Storage values") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};
    REQUIRE(s.getStorage("A_c1") == dbl_approx(1.0));
    s.setStorage("A_c1", 2.5);
    REQUIRE(s.getStorage("A_c1") == dbl_approx(2.5));
    // S=0 is now valid
    s.setStorage("A_c1", 0.0);
    REQUIRE(s.getStorage("A_c1") == dbl_approx(0.0));
    // restore to positive for remaining tests
    s.setStorage("A_c1", 2.5);
    REQUIRE(s.getStorage("A_c1") == dbl_approx(2.5));
    // ignore negative values
    s.setStorage("A_c1", -4.0);
    REQUIRE(s.getStorage("A_c1") == dbl_approx(2.5));
    // S=0 persists via SBML annotations
    s.setStorage("A_c1", 0.0);
    auto xml0{m.getXml().toStdString()};
    model::Model m0;
    m0.importSBMLString(xml0);
    REQUIRE(m0.getSpecies().getStorage("A_c1") == dbl_approx(0.0));
    // positive value persists via SBML annotations
    s.setStorage("A_c1", 2.5);
    auto xml{m.getXml().toStdString()};
    model::Model m2;
    m2.importSBMLString(xml);
    REQUIRE(m2.getSpecies().getStorage("A_c1") == dbl_approx(2.5));
    // unknown species defaults to 1
    REQUIRE(m2.getSpecies().getStorage("does_not_exist") == dbl_approx(1.0));
  }
  SECTION("Cross-diffusion constants") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &s{m.getSpecies()};

    REQUIRE(s.getCrossDiffusionConstants("B_c1").empty());
    REQUIRE(s.getCrossDiffusionConstant("B_c1", "A_c1").isEmpty());
    REQUIRE(s.isValidCrossDiffusionExpression("B_c1", "A_c1 + param"));
    REQUIRE(!s.isValidCrossDiffusionExpression("B_c1", ""));
    REQUIRE(!s.isValidCrossDiffusionExpression("B_c1", "A_c1 + *"));
    REQUIRE(!s.isValidCrossDiffusionExpression("does_not_exist", "1"));

    s.setCrossDiffusionConstant("B_c1", "A_c1", " 1 + A_c1 + param ");
    REQUIRE(
        symEq(s.getCrossDiffusionConstant("B_c1", "A_c1"), "1 + A_c1 + param"));
    REQUIRE(s.getCrossDiffusionConstants("B_c1").size() == 1);

    // invalid target/source pairs are ignored
    s.setHasUnsavedChanges(false);
    s.setCrossDiffusionConstant("B_c1", "B_c1", "2");
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.setCrossDiffusionConstant("B_c1", "A_c2", "2");
    REQUIRE(s.getCrossDiffusionConstants("B_c1").size() == 1);

    // invalid expression is ignored
    s.setCrossDiffusionConstant("B_c1", "A_c1", "A_c1 + *");
    REQUIRE(
        symEq(s.getCrossDiffusionConstant("B_c1", "A_c1"), "1 + A_c1 + param"));

    // zero removes term
    s.setCrossDiffusionConstant("B_c1", "A_c1", "0");
    REQUIRE(s.getCrossDiffusionConstant("B_c1", "A_c1").isEmpty());
    REQUIRE(s.getCrossDiffusionConstants("B_c1").empty());

    // removing absent term is a no-op
    s.setHasUnsavedChanges(false);
    s.removeCrossDiffusionConstant("B_c1", "A_c1");
    REQUIRE(s.getHasUnsavedChanges() == false);

    // relocation removes invalid cross-compartment entries
    s.setCrossDiffusionConstant("A_c2", "B_c2", "3");
    s.setCrossDiffusionConstant("B_c2", "A_c2", "4");
    REQUIRE(!s.getCrossDiffusionConstant("A_c2", "B_c2").isEmpty());
    REQUIRE(!s.getCrossDiffusionConstant("B_c2", "A_c2").isEmpty());
    s.setCompartment("A_c2", "c1");
    REQUIRE(s.getCrossDiffusionConstant("A_c2", "B_c2").isEmpty());
    REQUIRE(s.getCrossDiffusionConstant("B_c2", "A_c2").isEmpty());

    // removing a species removes outgoing and incoming terms
    s.setCrossDiffusionConstant("B_c1", "A_c1", "5");
    s.setCrossDiffusionConstant("A_c1", "B_c1", "6");
    REQUIRE(!s.getCrossDiffusionConstant("B_c1", "A_c1").isEmpty());
    REQUIRE(!s.getCrossDiffusionConstant("A_c1", "B_c1").isEmpty());
    s.remove("A_c1");
    REQUIRE(s.getCrossDiffusionConstant("B_c1", "A_c1").isEmpty());
    REQUIRE(s.getCrossDiffusionConstants("A_c1").empty());
  }
}
