#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("Optimize ABtoC model for zero concentration of A",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.nParticles = 3;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.05,
       0.21});
  // optimization cost: absolute difference of concentration of species A from
  // zero, after simulating for time 10
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "A",
                                      10.0,
                                      1.0,
                                      0,
                                      0,
                                      {}});
  model.getOptimizeOptions() = optimizeOptions;
  sme::simulate::Optimization optimization(model);
  double cost{optimization.getFitness()[0]};
  double k1{optimization.getParams()[0][0]};
  for (std::size_t i = 1; i < 3; ++i) {
    optimization.evolve();
    REQUIRE(optimization.getIterations() == i);
    // cost should decrease or stay the same with each iteration
    REQUIRE(optimization.getFitness()[0] <= cost);
    // k1 should increase to minimize concentration of A
    REQUIRE(optimization.getParams().back()[0] >= k1);
    cost = optimization.getFitness()[0];
    k1 = optimization.getParams().back()[0];
  }
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.1));
  optimization.applyParametersToModel(&model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") == dbl_approx(k1));
}

TEST_CASE("Optimize ABtoC model for zero concentration of C",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.nParticles = 3;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.02,
       0.88});
  // optimization cost: absolute difference of concentration of species C from
  // zero, after simulating for time 1
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "C",
                                      1.0,
                                      0.23,
                                      0,
                                      2,
                                      {}});
  model.getOptimizeOptions() = optimizeOptions;
  sme::simulate::Optimization optimization(model);
  double cost{optimization.getFitness()[0]};
  double k1{optimization.getParams()[0][0]};
  for (std::size_t i = 1; i < 3; ++i) {
    optimization.evolve();
    REQUIRE(optimization.getIterations() == i);
    // cost should decrease or stay the same with each iteration
    REQUIRE(optimization.getFitness()[0] <= cost);
    // k1 should decrease to minimize concentration of C
    REQUIRE(optimization.getParams().back()[0] <= k1);
    cost = optimization.getFitness()[0];
    k1 = optimization.getParams().back()[0];
  }
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.1));
  optimization.applyParametersToModel(&model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") == dbl_approx(k1));
}

TEST_CASE("Save and load model with optimization settings",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.nParticles = 3;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "optParamName", "k1",
       "r1", 0.02, 0.88});
  // optimization cost: absolute difference of concentration of species C from
  // zero, after simulating for time 1
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "optCostName",
                                      "C",
                                      1.0,
                                      0.23,
                                      0,
                                      2,
                                      {}});
  model.getOptimizeOptions() = optimizeOptions;
  // export model as xml
  constexpr auto tempfilename{"test_optimize_load_save.xml"};
  model.exportSBMLFile(tempfilename);
  // import model from xml, check optimization options are preserved
  sme::model::Model reloadedModel;
  reloadedModel.importFile(tempfilename);
  const auto &options{reloadedModel.getOptimizeOptions()};
  REQUIRE(optimizeOptions.nParticles == 3);
  REQUIRE(optimizeOptions.optCosts.size() == 1);
  REQUIRE(optimizeOptions.optCosts[0].optCostType ==
          sme::simulate::OptCostType::Concentration);
  REQUIRE(optimizeOptions.optCosts[0].optCostDiffType ==
          simulate::OptCostDiffType::Absolute);
  REQUIRE(optimizeOptions.optCosts[0].name == "optCostName");
  REQUIRE(optimizeOptions.optCosts[0].id == "C");
  REQUIRE(optimizeOptions.optCosts[0].simulationTime == dbl_approx(1.0));
  REQUIRE(optimizeOptions.optCosts[0].weight == dbl_approx(0.23));
  REQUIRE(optimizeOptions.optCosts[0].compartmentIndex == 0);
  REQUIRE(optimizeOptions.optCosts[0].speciesIndex == 2);
  REQUIRE(optimizeOptions.optCosts[0].targetValues.empty());
  REQUIRE(optimizeOptions.optParams.size() == 1);
  REQUIRE(optimizeOptions.optParams[0].optParamType ==
          sme::simulate::OptParamType::ReactionParameter);
  REQUIRE(optimizeOptions.optParams[0].name == "optParamName");
  REQUIRE(optimizeOptions.optParams[0].id == "k1");
  REQUIRE(optimizeOptions.optParams[0].parentId == "r1");
  REQUIRE(optimizeOptions.optParams[0].lowerBound == dbl_approx(0.02));
  REQUIRE(optimizeOptions.optParams[0].upperBound == dbl_approx(0.88));
}
