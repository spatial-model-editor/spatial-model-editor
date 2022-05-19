#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "optimize_impl.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"
#include "sme/utils.hpp"
#include <algorithm>

using namespace sme;
using namespace sme::test;

TEST_CASE("Optimize applyParameters: parameters",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::VerySimpleModel)};
  // optimization parameter: param parameter
  simulate::OptParam optParam{
      simulate::OptParamType::ModelParameter, "name", "param", "", 0.66, 0.99};
  REQUIRE(model.getParameters().getExpression("param").toDouble() ==
          dbl_approx(1));
  simulate::applyParameters({0.123}, {optParam}, &model);
  REQUIRE(model.getParameters().getExpression("param").toDouble() ==
          dbl_approx(0.123));
  simulate::applyParameters({-99}, {optParam}, &model);
  REQUIRE(model.getParameters().getExpression("param").toDouble() ==
          dbl_approx(-99));
}

TEST_CASE("Optimize applyParameters: reaction parameters",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  // optimization parameter: k1 parameter of reaction r1
  simulate::OptParam optParam{simulate::OptParamType::ReactionParameter,
                              "name",
                              "k1",
                              "r1",
                              0.05,
                              0.21};
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.1));
  simulate::applyParameters({0.123}, {optParam}, &model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.123));
  simulate::applyParameters({-99}, {optParam}, &model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(-99));
}

TEST_CASE("Optimize calculateCosts: zero or no target values",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  simulate::Simulation sim(model);
  auto zeroTarget{
      model.getSpecies().getField("A")->getConcentrationImageArray()};
  // make explicit target of zero for all pixels
  std::fill(zeroTarget.begin(), zeroTarget.end(), 0.0);

  // cost: absolute difference of concentration of species A from zero
  simulate::OptCost optCostConc{};
  optCostConc.optCostType = simulate::OptCostType::Concentration;
  optCostConc.optCostDiffType = simulate::OptCostDiffType::Absolute;
  optCostConc.name = "name";
  optCostConc.id = "A";
  optCostConc.scaleFactor = 1.0;
  optCostConc.compartmentIndex = 0;
  optCostConc.speciesIndex = 0;
  optCostConc.targetValues = {};
  // cost: absolute difference of dcdt of species A from zero
  simulate::OptCost optCostDcdt{};
  optCostDcdt.optCostType = simulate::OptCostType::ConcentrationDcdt;
  optCostDcdt.optCostDiffType = simulate::OptCostDiffType::Absolute;
  optCostDcdt.name = "name";
  optCostDcdt.id = "A";
  optCostDcdt.scaleFactor = 1.0;
  optCostDcdt.compartmentIndex = 0;
  optCostDcdt.speciesIndex = 0;
  optCostDcdt.targetValues = {};

  // costs are zero if no OptCost supplied:
  REQUIRE(simulate::calculateCosts({}, {}, sim) == dbl_approx(0.0));

  // sum over initial concentration of A to get costConc vs 0
  auto conc{model.getSpecies().getField("A")->getConcentration()};
  double cost{common::sum(conc)};
  REQUIRE(simulate::calculateCosts({optCostConc}, {0}, sim) ==
          dbl_approx(cost));
  // repeat with explicit vector of zeros as target
  optCostConc.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostConc}, {0}, sim) ==
          dbl_approx(cost));
  optCostConc.targetValues.clear();
  // initial dcdt without simulating is zero
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim) == 0.0);
  // repeat with explicit vector of zeros as target
  optCostDcdt.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim) == 0.0);
  optCostDcdt.targetValues.clear();
  sim.doTimesteps(1.0);
  // after simulation non-zero
  auto c{simulate::calculateCosts({optCostDcdt}, {0}, sim)};
  REQUIRE(c > 0.0);
  // check equal to explicit zeros target value
  optCostDcdt.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim) == dbl_approx(c));
  optCostDcdt.targetValues.clear();

  // if concentration A is set to constant, conc cost is just npixels * that
  // value
  model.getSimulationData().clear();
  model.getSpecies().setInitialConcentration("A", 0.73);
  model.getSpecies().setInitialConcentration("B", 1.24);
  simulate::Simulation sim2(model);
  auto c1{static_cast<double>(conc.size()) * 0.73};
  REQUIRE(simulate::calculateCosts({optCostConc}, {0}, sim2) ==
          dbl_approx(c1).epsilon(1e-13));
  // repeat with explicit vector of zeros as target
  optCostConc.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostConc}, {0}, sim2) ==
          dbl_approx(c1).epsilon(1e-13));
  optCostConc.targetValues.clear();
  // if concentration B is also constant, then initial dA/dt is approx -A B k1
  sim2.doTimesteps(1e-8);
  auto c2{static_cast<double>(conc.size()) * 0.73 * 1.24 * 0.1};
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim2) ==
          dbl_approx(c2).epsilon(1e-8));
  // repeat with explicit vector of zeros as target
  optCostDcdt.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim2) ==
          dbl_approx(c2).epsilon(1e-8));
  optCostDcdt.targetValues.clear();

  // if concentration A is set to zero, conc cost & dcdt cost are zero
  model.getSimulationData().clear();
  model.getSpecies().setInitialConcentration("A", 0.0);
  simulate::Simulation sim3(model);
  sim3.doTimesteps(1.0);
  REQUIRE(simulate::calculateCosts({optCostConc}, {0}, sim3) ==
          dbl_approx(0.0));
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim3) ==
          dbl_approx(0.0));
  // repeat with explicit vector of zeros as target
  optCostConc.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostConc}, {0}, sim3) ==
          dbl_approx(0.0));
  optCostConc.targetValues.clear();
  optCostDcdt.targetValues = zeroTarget;
  REQUIRE(simulate::calculateCosts({optCostDcdt}, {0}, sim3) ==
          dbl_approx(0.0));
  optCostDcdt.targetValues.clear();
}

TEST_CASE("Optimize calculateCosts: target values",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  simulate::Simulation sim(model);
  // make explicit target of 2 for all pixels in compartment
  const auto *comp{model.getSpecies().getField("A")->getCompartment()};
  const auto compImgWidth{comp->getCompartmentImage().width()};
  const auto compImgHeight{comp->getCompartmentImage().height()};
  std::vector<double> target(compImgWidth * compImgHeight, 0.0);
  constexpr double targetPixel{2.0};
  for (const auto &pixel : comp->getPixels()) {
    target[pixel.x() + compImgWidth * (compImgHeight - 1 - pixel.y())] =
        targetPixel;
  }
  double targetSum{static_cast<double>(comp->nPixels() * targetPixel)};
  constexpr double epsilon{3.4e-11};

  // cost: absolute difference of concentration of species A from zero
  simulate::OptCost optCostConcAbs{};
  optCostConcAbs.optCostType = simulate::OptCostType::Concentration;
  optCostConcAbs.optCostDiffType = simulate::OptCostDiffType::Absolute;
  optCostConcAbs.name = "name";
  optCostConcAbs.id = "A";
  optCostConcAbs.simulationTime = 100.0;
  optCostConcAbs.scaleFactor = 1.0;
  optCostConcAbs.compartmentIndex = 0;
  optCostConcAbs.speciesIndex = 0;
  optCostConcAbs.targetValues = target;
  // cost: absolute difference of dcdt of species A from zero
  simulate::OptCost optCostDcdtAbs{};
  optCostDcdtAbs.optCostType = simulate::OptCostType::ConcentrationDcdt;
  optCostDcdtAbs.optCostDiffType = simulate::OptCostDiffType::Absolute;
  optCostDcdtAbs.name = "name";
  optCostDcdtAbs.id = "A";
  optCostDcdtAbs.simulationTime = 100.0 * (1 + 1e-14);
  optCostDcdtAbs.scaleFactor = 1.0;
  optCostDcdtAbs.compartmentIndex = 0;
  optCostDcdtAbs.speciesIndex = 0;
  optCostDcdtAbs.targetValues = target;
  // cost: relative difference of concentration of species A from zero
  simulate::OptCost optCostConcRel{};
  optCostConcRel.optCostType = simulate::OptCostType::Concentration;
  optCostConcRel.optCostDiffType = simulate::OptCostDiffType::Relative;
  optCostConcRel.name = "name";
  optCostConcRel.id = "A";
  optCostConcRel.simulationTime = 100.0 * (1 - 1e-14);
  optCostConcRel.scaleFactor = 1.0;
  optCostConcRel.compartmentIndex = 0;
  optCostConcRel.speciesIndex = 0;
  optCostConcRel.targetValues = target;
  optCostConcRel.epsilon = epsilon;
  // cost: relative difference of dcdt of species A from zero
  simulate::OptCost optCostDcdtRel{};
  optCostDcdtRel.optCostType = simulate::OptCostType::ConcentrationDcdt;
  optCostDcdtRel.optCostDiffType = simulate::OptCostDiffType::Relative;
  optCostDcdtRel.name = "name";
  optCostDcdtRel.id = "A";
  optCostDcdtRel.simulationTime = 100.0;
  optCostDcdtRel.scaleFactor = 1.0;
  optCostDcdtRel.compartmentIndex = 0;
  optCostDcdtRel.speciesIndex = 0;
  optCostDcdtRel.targetValues = target;
  optCostDcdtRel.epsilon = epsilon;

  // sum over initial concentration of A to get costConc vs 2
  auto conc{model.getSpecies().getField("A")->getConcentration()};
  double cost{targetSum - common::sum(conc)};
  REQUIRE(simulate::calculateCosts({optCostConcAbs}, {0}, sim) ==
          dbl_approx(cost));
  // relative difference just a rescaling since target is uniform:
  // diff/(targetPixel+epsilon)
  REQUIRE(simulate::calculateCosts({optCostConcRel}, {0}, sim) ==
          dbl_approx(cost / (targetPixel + epsilon)).epsilon(1e-13));

  // initial dcdt without simulating is zero, so cost = |0 - targetSum|
  REQUIRE(simulate::calculateCosts({optCostDcdtAbs}, {0}, sim) ==
          dbl_approx(targetSum));
  REQUIRE(simulate::calculateCosts({optCostDcdtRel}, {0}, sim) ==
          dbl_approx(targetSum / (targetPixel + epsilon)).epsilon(1e-13));
  sim.doTimesteps(1.0);
  // after simulation dcdt is negative for A, so cost increased
  REQUIRE(simulate::calculateCosts({optCostDcdtAbs}, {0}, sim) > targetSum);
  // absolute & relative still related by constant factor since relative uses
  // target value in denominator which is the same everywhere
  REQUIRE(simulate::calculateCosts({optCostDcdtRel}, {0}, sim) ==
          dbl_approx(simulate::calculateCosts({optCostDcdtAbs}, {0}, sim) /
                     (targetPixel + epsilon))
              .epsilon(1e-13));

  // if concentration A is set to target, conc cost is zero
  model.getSimulationData().clear();
  model.getSpecies().setInitialConcentration("A", targetPixel);
  double concB{1.254};
  model.getSpecies().setInitialConcentration("B", concB);
  simulate::Simulation sim2(model);
  REQUIRE(simulate::calculateCosts({optCostConcAbs}, {0}, sim2) ==
          dbl_approx(0));
  REQUIRE(simulate::calculateCosts({optCostConcRel}, {0}, sim2) ==
          dbl_approx(0));
  // if concentration B is also uniform, then initial dA/dt = - A B k1
  sim2.doTimesteps(1e-8);
  double dadt{-targetSum * concB * 0.1};
  REQUIRE(simulate::calculateCosts({optCostDcdtAbs}, {0}, sim2) ==
          dbl_approx(targetSum - dadt).epsilon(1e-8));
  REQUIRE(
      simulate::calculateCosts({optCostDcdtRel}, {0}, sim2) ==
      dbl_approx((targetSum - dadt) / (targetPixel + epsilon)).epsilon(1e-8));

  // if concentration A is set to zero, conc & dcdt are zero
  model.getSimulationData().clear();
  model.getSpecies().setInitialConcentration("A", 0.0);
  simulate::Simulation sim3(model);
  sim3.doTimesteps(1.0);
  REQUIRE(simulate::calculateCosts({optCostConcAbs}, {0}, sim3) ==
          dbl_approx(targetSum));
  REQUIRE(simulate::calculateCosts({optCostDcdtAbs}, {0}, sim3) ==
          dbl_approx(targetSum));
  REQUIRE(simulate::calculateCosts({optCostConcRel}, {0}, sim3) ==
          dbl_approx(targetSum / (targetPixel + epsilon)).epsilon(1e-13));
  REQUIRE(simulate::calculateCosts({optCostDcdtRel}, {0}, sim3) ==
          dbl_approx(targetSum / (targetPixel + epsilon)).epsilon(1e-13));
}

TEST_CASE("Optimize calculateCosts: invalid target values",
          "[core/simulate/optimize][core/simulate][core][optimize][invalid]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  auto correctSize{
      model.getSpecies().getField("A")->getConcentrationImageArray().size()};
  simulate::Simulation sim(model);
  simulate::OptCost optCostInvalid{simulate::OptCostType::Concentration,
                                   simulate::OptCostDiffType::Absolute,
                                   "name",
                                   "A",
                                   100.0,
                                   1.0,
                                   0,
                                   0,
                                   {}};
  // correct size
  optCostInvalid.targetValues = std::vector<double>(correctSize, 1.5);
  REQUIRE_NOTHROW(simulate::calculateCosts({optCostInvalid}, {0}, sim));
  // too small
  optCostInvalid.targetValues = {{1.0, 2.0}};
  REQUIRE_THROWS(simulate::calculateCosts({optCostInvalid}, {0}, sim));
  optCostInvalid.targetValues = std::vector<double>(correctSize - 1, 1.3);
  REQUIRE_THROWS(simulate::calculateCosts({optCostInvalid}, {0}, sim));
  // too large
  optCostInvalid.targetValues = std::vector<double>(correctSize + 1, 1.5);
  REQUIRE_THROWS(simulate::calculateCosts({optCostInvalid}, {0}, sim));
}
