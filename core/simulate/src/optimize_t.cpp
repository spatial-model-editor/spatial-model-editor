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
      {sme::simulate::OptParamType::ReactionParameter, "k1", "r1", 0.05, 0.21});
  // optimization cost: absolute difference of concentration of species A from
  // zero
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      1.0,
                                      0,
                                      0,
                                      {}});
  // simulation time: 10
  optimizeOptions.simulationTime = 10.0;
  sme::simulate::Optimization optimization(model, optimizeOptions);
  double cost{optimization.fitness()[0]};
  double k1{optimization.params()[0]};
  for (std::size_t i = 1; i < 3; ++i) {
    optimization.evolve();
    REQUIRE(optimization.iterations() == i);
    // cost should decrease or stay the same with each iteration
    REQUIRE(optimization.fitness()[0] <= cost);
    // k1 should increase to minimize concentration of A
    REQUIRE(optimization.params()[0] >= k1);
    cost = optimization.fitness()[0];
    k1 = optimization.params()[0];
  }
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.1));
  optimization.applyParametersToModel(&model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") == dbl_approx(k1));
}

TEST_CASE("Optimize ABtoC model for zero concentration of C",
          "[Q][core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.nParticles = 3;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "k1", "r1", 0.02, 0.88});
  // optimization cost: absolute difference of concentration of species C from
  // zero
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      0.23,
                                      0,
                                      2,
                                      {}});
  // simulation time: 1
  optimizeOptions.simulationTime = 1.0;
  sme::simulate::Optimization optimization(model, optimizeOptions);
  double cost{optimization.fitness()[0]};
  double k1{optimization.params()[0]};
  for (std::size_t i = 1; i < 3; ++i) {
    optimization.evolve();
    REQUIRE(optimization.iterations() == i);
    // cost should decrease or stay the same with each iteration
    REQUIRE(optimization.fitness()[0] <= cost);
    // k1 should decrease to minimize concentration of C
    REQUIRE(optimization.params()[0] <= k1);
    cost = optimization.fitness()[0];
    k1 = optimization.params()[0];
  }
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.1));
  optimization.applyParametersToModel(&model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") == dbl_approx(k1));
}
