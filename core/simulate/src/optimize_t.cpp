#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"

using namespace sme;
using namespace sme::test;

template <typename T> static bool is_sorted_ascending(const std::vector<T> &v) {
  return std::is_sorted(v.begin(), v.end());
}

template <typename T>
static bool is_sorted_descending(const std::vector<T> &v) {
  return std::is_sorted(v.begin(), v.end(), [](T a, T b) { return a > b; });
}

struct TestOptimization {

  sme::model::Model model;

  TestOptimization() {
    model = getExampleModel(Mod::ABtoC);

    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::Pixel;
    sme::simulate::OptimizeOptions optimizeOptions;
    optimizeOptions.optAlgorithm.islands = 1;
    optimizeOptions.optAlgorithm.population = 3;
    // optimization parameter: k1 parameter of reaction r1
    optimizeOptions.optParams.push_back(
        {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1",
         0.02, 0.88});
    // optimization cost: absolute difference of concentration of species C from
    // zero, after simulating for time 1
    optimizeOptions.optCosts.push_back(
        {sme::simulate::OptCostType::Concentration,
         simulate::OptCostDiffType::Absolute,
         "name1",
         "C",
         1.0,
         0.23,
         0,
         2,
         {}});
    // optimization cost: absolute difference of concentration of species A from
    // 2, after simulating for time 1
    // make explicit target of 2 for all pixels in compartment
    const auto *comp{model.getSpecies().getField("A")->getCompartment()};
    const auto compImgWidth{comp->getCompartmentImages()[0].width()};
    const auto compImgHeight{comp->getCompartmentImages()[0].height()};
    std::vector<double> target(
        static_cast<std::size_t>(compImgWidth * compImgHeight), 0.0);
    constexpr double targetPixel{2.0};
    for (const auto &voxel : comp->getVoxels()) {
      target[static_cast<std::size_t>(voxel.p.x()) +
             static_cast<std::size_t>(compImgWidth) *
                 static_cast<std::size_t>(compImgHeight - 1 - voxel.p.y())] =
          targetPixel;
    }
    optimizeOptions.optCosts.push_back(
        {sme::simulate::OptCostType::Concentration,
         simulate::OptCostDiffType::Absolute, "name2", "A", 1.0, 0.23, 0, 2,
         target});

    model.getOptimizeOptions() = optimizeOptions;
  }
};

TEST_CASE("Invalid population sizes",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.islands = 1;
  optimizeOptions.optAlgorithm.population = 2;
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.05,
       0.21});
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "A",
                                      0.1,
                                      1.0,
                                      0,
                                      0,
                                      {}});
  {
    optimizeOptions.optAlgorithm.optAlgorithmType =
        sme::simulate::OptAlgorithmType::PSO;
    optimizeOptions.optAlgorithm.population = 1;
    model.getOptimizeOptions() = optimizeOptions;
    sme::simulate::Optimization optimization(model);
    optimization.evolve();
    REQUIRE(optimization.getErrorMessage() ==
            "Invalid optimization population size, can't be less than 2");
  }
  {
    optimizeOptions.optAlgorithm.optAlgorithmType =
        sme::simulate::OptAlgorithmType::pDE;
    optimizeOptions.optAlgorithm.population = 6;
    model.getOptimizeOptions() = optimizeOptions;
    sme::simulate::Optimization optimization(model);
    optimization.evolve();
    REQUIRE(QString(optimization.getErrorMessage().c_str()).last(11) ==
            "6 detected\n");
  }
  {
    optimizeOptions.optAlgorithm.optAlgorithmType =
        sme::simulate::OptAlgorithmType::gaco;
    optimizeOptions.optAlgorithm.population = 5;
    model.getOptimizeOptions() = optimizeOptions;
    sme::simulate::Optimization optimization(model);
    optimization.evolve();
    REQUIRE(QString(optimization.getErrorMessage().c_str()).last(16) ==
            "population size\n");
  }
}

TEST_CASE("Optimize ABtoC with all algorithms for zero concentration of A",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.islands = 1;
  optimizeOptions.optAlgorithm.population = 2;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.05,
       0.21});
  // optimization cost: absolute difference of concentration of species A from
  // zero, after simulating for time 0.1
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "A",
                                      0.1,
                                      1.0,
                                      0,
                                      0,
                                      {}});

  for (auto optAlgorithmType : sme::simulate::optAlgorithmTypes) {
    CAPTURE(optAlgorithmType);
    // some algos need larger populations
    if (optAlgorithmType == sme::simulate::OptAlgorithmType::DE) {
      optimizeOptions.optAlgorithm.population = 5;
    } else if (optAlgorithmType == sme::simulate::OptAlgorithmType::iDE) {
      optimizeOptions.optAlgorithm.population = 7;
    } else if (optAlgorithmType == sme::simulate::OptAlgorithmType::jDE) {
      optimizeOptions.optAlgorithm.population = 7;
    } else if (optAlgorithmType == sme::simulate::OptAlgorithmType::pDE) {
      optimizeOptions.optAlgorithm.population = 7;
    } else if (optAlgorithmType == sme::simulate::OptAlgorithmType::gaco) {
      optimizeOptions.optAlgorithm.population = 7;
    } else {
      optimizeOptions.optAlgorithm.population = 2;
    }
    optimizeOptions.optAlgorithm.optAlgorithmType = optAlgorithmType;
    model.getOptimizeOptions() = optimizeOptions;
    model.getReactions().setParameterValue("r1", "k1", 0.1);
    sme::simulate::Optimization optimization(model);

    optimization.evolve();
    REQUIRE(optimization.getIterations() == 1);
    REQUIRE(optimization.getErrorMessage().empty());
    REQUIRE(optimization.getFitness().size() == 2);
    REQUIRE(optimization.getParams().size() == 2);
    REQUIRE(optimization.getParams()[0].size() == 1);
    // cost should decrease or stay the same with each iteration
    REQUIRE(optimization.getFitness()[1] <= optimization.getFitness()[0]);
    // k1 should increase to minimize concentration of A
    REQUIRE(optimization.getParams()[1][0] >= optimization.getParams()[0][0]);
    REQUIRE(optimization.getIsRunning() == false);
  }
}

TEST_CASE(
    "Optimize ABtoC with all algorithms, multiple evolve calls, for zero "
    "concentration of A",
    "[core/simulate/optimize][core/simulate][core][optimize][expensive]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.islands = 2;
  optimizeOptions.optAlgorithm.population = 7;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.05,
       0.21});
  // optimization cost: absolute difference of concentration of species A from
  // zero, after simulating for time 2
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "A",
                                      2.0,
                                      1.0,
                                      0,
                                      0,
                                      {}});
  for (auto optAlgorithmType : sme::simulate::optAlgorithmTypes) {
    CAPTURE(optAlgorithmType);
    optimizeOptions.optAlgorithm.optAlgorithmType = optAlgorithmType;
    model.getOptimizeOptions() = optimizeOptions;
    model.getReactions().setParameterValue("r1", "k1", 0.1);
    sme::simulate::Optimization optimization(model);
    for (std::size_t i = 1; i < 3; ++i) {
      optimization.evolve();
      REQUIRE(optimization.getErrorMessage().empty());
      REQUIRE(optimization.getIterations() == i);
      REQUIRE(optimization.getFitness().size() == i + 1);
      REQUIRE(optimization.getParams().size() == i + 1);
      // cost should decrease or stay the same with each iteration
      REQUIRE(is_sorted_descending(optimization.getFitness()));
      // k1 should increase to minimize concentration of A
      std::vector<double> k1;
      for (const auto &param : optimization.getParams()) {
        k1.push_back(param[0]);
      }
      REQUIRE(is_sorted_ascending(k1));
    }
    REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
            dbl_approx(0.1));
    optimization.applyParametersToModel(&model);
    REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
            dbl_approx(optimization.getParams().back()[0]));
  }
}

TEST_CASE("Optimize ABtoC model for zero concentration of C",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.islands = 1;
  optimizeOptions.optAlgorithm.population = 3;
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
  for (std::size_t i = 1; i < 3; ++i) {
    optimization.evolve();
    REQUIRE(optimization.getErrorMessage().empty());
    REQUIRE(optimization.getIterations() == i);
    // cost should decrease or stay the same with each iteration
    REQUIRE(is_sorted_descending(optimization.getFitness()));
    // k1 should decrease to minimize concentration of C
    std::vector<double> k1;
    for (const auto &param : optimization.getParams()) {
      k1.push_back(param[0]);
    }
    REQUIRE(is_sorted_descending(k1));
  }
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(0.1));
  optimization.applyParametersToModel(&model);
  REQUIRE(model.getReactions().getParameterValue("r1", "k1") ==
          dbl_approx(optimization.getParams().back()[0]));
}

TEST_CASE("setBestResults and getUpdatedBestResultImage",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.islands = 1;
  optimizeOptions.optAlgorithm.population = 3;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.02,
       0.88});
  // optimization cost: absolute difference of concentration of species C from
  // zero, after simulating for time 1
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name1",
                                      "C",
                                      1.0,
                                      0.23,
                                      0,
                                      2,
                                      {}});
  // optimization cost: absolute difference of concentration of species A from
  // 2, after simulating for time 1
  // make explicit target of 2 for all pixels in compartment
  const auto *comp{model.getSpecies().getField("A")->getCompartment()};
  const auto compImgWidth{comp->getCompartmentImages()[0].width()};
  const auto compImgHeight{comp->getCompartmentImages()[0].height()};
  std::vector<double> target(
      static_cast<std::size_t>(compImgWidth * compImgHeight), 0.0);
  constexpr double targetPixel{2.0};
  for (const auto &voxel : comp->getVoxels()) {
    target[static_cast<std::size_t>(voxel.p.x()) +
           static_cast<std::size_t>(compImgWidth) *
               static_cast<std::size_t>(compImgHeight - 1 - voxel.p.y())] =
        targetPixel;
  }
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name2", "A", 1.0, 0.23, 0, 2, target});
  model.getOptimizeOptions() = optimizeOptions;
  sme::simulate::Optimization optimization(model);
  // initially no parameters or fitness info
  REQUIRE(optimization.applyParametersToModel(&model) == false);
  REQUIRE(optimization.getParams().empty());
  REQUIRE(optimization.getFitness().empty());
  REQUIRE(optimization.getIterations() == 0);
  // first target is zero C (i.e. black everywhere)
  REQUIRE(optimization.getTargetImage(0).volume().depth() == 1);
  REQUIRE(optimization.getTargetImage(0)[0].pixel(0, 0) == qRgb(0, 0, 0));
  REQUIRE(optimization.getTargetImage(0)[0].pixel(40, 40) == qRgb(0, 0, 0));
  // second target is constant & non-zero A (i.e. white in compartment, black
  // outside)
  REQUIRE(optimization.getTargetImage(1).volume().depth() == 1);
  REQUIRE(optimization.getTargetImage(1)[0].pixel(0, 0) == qRgb(0, 0, 0));
  for (const auto &voxel : comp->getVoxels()) {
    REQUIRE(optimization.getTargetImage(1)[voxel.z].pixel(voxel.p) ==
            qRgb(255, 255, 255));
  }
  // no best result images yet
  REQUIRE(optimization.getUpdatedBestResultImage(0).has_value() == false);
  REQUIRE(optimization.getUpdatedBestResultImage(1).has_value() == false);
  // do a single evolution
  optimization.evolve();
  REQUIRE(optimization.getIterations() == 1);
  // first evolution also calculates fitness for initial random params, so we
  // have two params & fitness values
  REQUIRE(optimization.getParams().size() == 2);
  REQUIRE(optimization.getFitness().size() == 2);
  // call getUpdatedBestResultImage with new results
  auto img0{optimization.getUpdatedBestResultImage(0).value()};
  REQUIRE(img0.volume().depth() == 1);
  REQUIRE(img0[0].size() == QSize(100, 100));
  // call getUpdatedBestResultImage again with same index without new results
  REQUIRE(optimization.getUpdatedBestResultImage(0).has_value() == false);
  // different index, new results
  auto img1{optimization.getUpdatedBestResultImage(1).value()};
  REQUIRE(img1.volume().depth() == 1);
  REQUIRE(img1[0].size() == QSize(100, 100));
  // call getUpdatedBestResultImage again with same index without new results
  REQUIRE(optimization.getUpdatedBestResultImage(1).has_value() == false);
  // call getUpdatedBestResultImage again with *different* index without new
  // results
  REQUIRE(optimization.getUpdatedBestResultImage(0).has_value() == true);
  REQUIRE(optimization.getUpdatedBestResultImage(1).has_value() == true);
  REQUIRE(optimization.getUpdatedBestResultImage(1).has_value() == false);

  // make a set of results
  constexpr double resultPixel{1.2};
  std::vector<double> result(
      static_cast<std::size_t>(compImgWidth * compImgHeight), 0.0);
  for (const auto &voxel : comp->getVoxels()) {
    result[static_cast<std::size_t>(voxel.p.x()) +
           static_cast<std::size_t>(compImgWidth) *
               static_cast<std::size_t>(compImgHeight - 1 - voxel.p.y())] =
        resultPixel;
  }
  // setBestResults with inferior fitness is a no-op
  auto worseFitness{optimization.getFitness().back() + 1.0};
  REQUIRE(optimization.setBestResults(
              worseFitness,
              std::vector<std::vector<double>>{{result, result}}) == false);
  // call getUpdatedBestResultImage again with same index without new results
  REQUIRE(optimization.getUpdatedBestResultImage(1).has_value() == false);
  // setBestResults with better fitness
  auto betterFitness{optimization.getFitness().back() - 1.0};
  REQUIRE(optimization.setBestResults(
              betterFitness,
              std::vector<std::vector<double>>{{result, result}}) == true);
  // img0 is compared to a zero target, so image normalised to its own max value
  img0 = optimization.getUpdatedBestResultImage(0).value();
  REQUIRE(img0.volume().depth() == 1);
  REQUIRE(img0[0].size() == QSize(100, 100));
  for (const auto &voxel : comp->getVoxels()) {
    REQUIRE(img0[voxel.z].pixel(voxel.p) == qRgb(255, 255, 255));
  }
  // call getUpdatedBestResultImage again without new results
  REQUIRE(optimization.getUpdatedBestResultImage(0).has_value() == false);
  // img1 is compared to a non-zero target, so it is normalised to the max value
  // of the target
  img1 = optimization.getUpdatedBestResultImage(1).value();
  REQUIRE(img1.volume().depth() == 1);
  REQUIRE(img1[0].size() == QSize(100, 100));
  auto maxColor{static_cast<int>(255.0 * resultPixel / targetPixel)};
  for (const auto &voxel : comp->getVoxels()) {
    REQUIRE(img1[voxel.z].pixel(voxel.p) == qRgb(maxColor, maxColor, maxColor));
  }
  // call getUpdatedBestResultImage again without new results
  REQUIRE(optimization.getUpdatedBestResultImage(1).has_value() == false);
}

TEST_CASE("Save and load model with optimization settings",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.optAlgorithmType =
      sme::simulate::OptAlgorithmType::ABC;
  optimizeOptions.optAlgorithm.islands = 1;
  optimizeOptions.optAlgorithm.population = 3;
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
  REQUIRE(options.optAlgorithm.optAlgorithmType ==
          sme::simulate::OptAlgorithmType::ABC);
  REQUIRE(options.optAlgorithm.islands == 1);
  REQUIRE(options.optAlgorithm.population == 3);
  REQUIRE(options.optCosts.size() == 1);
  REQUIRE(options.optCosts[0].optCostType ==
          sme::simulate::OptCostType::Concentration);
  REQUIRE(options.optCosts[0].optCostDiffType ==
          simulate::OptCostDiffType::Absolute);
  REQUIRE(options.optCosts[0].name == "optCostName");
  REQUIRE(options.optCosts[0].id == "C");
  REQUIRE(options.optCosts[0].simulationTime == dbl_approx(1.0));
  REQUIRE(options.optCosts[0].weight == dbl_approx(0.23));
  REQUIRE(options.optCosts[0].compartmentIndex == 0);
  REQUIRE(options.optCosts[0].speciesIndex == 2);
  REQUIRE(options.optCosts[0].targetValues.empty());
  REQUIRE(options.optParams.size() == 1);
  REQUIRE(options.optParams[0].optParamType ==
          sme::simulate::OptParamType::ReactionParameter);
  REQUIRE(options.optParams[0].name == "optParamName");
  REQUIRE(options.optParams[0].id == "k1");
  REQUIRE(options.optParams[0].parentId == "r1");
  REQUIRE(options.optParams[0].lowerBound == dbl_approx(0.02));
  REQUIRE(options.optParams[0].upperBound == dbl_approx(0.88));
}

TEST_CASE("Start long optimization in another thread & stop early",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions;
  optimizeOptions.optAlgorithm.islands = 2;
  optimizeOptions.optAlgorithm.population = 3;
  // optimization parameter: k1 parameter of reaction r1
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.05,
       0.21});
  // optimization cost: absolute difference of concentration of species A from
  // zero, after simulating for time 1e6
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "A",
                                      1e6,
                                      1.0,
                                      0,
                                      0,
                                      {}});
  model.getOptimizeOptions() = optimizeOptions;
  sme::simulate::Optimization optimization(model);
  // run optimization in another thread
  auto optSteps = std::async(std::launch::async,
                             &simulate::Optimization::evolve, &optimization, 1);
  // wait until it starts running
  while (!optimization.getIsRunning()) {
    sme::test::wait(10);
  }
  // request it to stop early
  optimization.requestStop();
  // this `.get()` blocks until optimization is finished
  REQUIRE(optSteps.get() >= 1);
  REQUIRE(optimization.getIsRunning() == false);
  REQUIRE(optimization.getIsStopping() == false);
  // stopped optimization before any simulation could complete, all fitness
  // calls should return max double
  REQUIRE(optimization.getFitness().size() >= 1);
  REQUIRE(optimization.getFitness().back() ==
          dbl_approx(std::numeric_limits<double>::max()));
}

TEST_CASE("optimizer_imageFunctions",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  TestOptimization optim;
  sme::simulate::Optimization optimizer(optim.model);

  optimizer.evolve(5);

  SECTION("getImageSize") {
    auto imgSize = optimizer.getImageSize();
    REQUIRE(imgSize.depth() == 1);
    REQUIRE(imgSize.width() == 100);
    REQUIRE(imgSize.height() == 100);
  }
  SECTION("getDifferenceImage") {
    auto idx = 1;
    auto img = optimizer.getDifferenceImage(idx);

    auto diff = std::vector<double>(10000, 0);

    std::ranges::transform(optimizer.getTargetValues(idx),
                           optimizer.getBestResultValues(idx), diff.begin(),
                           std::minus<double>());

    auto expected =
        sme::common::ImageStack(sme::common::Volume(100, 100, 1), diff,
                                *std::max_element(diff.begin(), diff.end()));
    REQUIRE(img[0] == expected[0]); // we only have one slice
  }
  SECTION("getTargetValues") {
    auto idx = 1;
    auto values = optimizer.getTargetValues(idx);
    REQUIRE(values.size() == 10000);
    REQUIRE(*std::ranges::min_element(values) == dbl_approx(0.0));
    REQUIRE(*std::ranges::max_element(values) == dbl_approx(2.0));
  }
  SECTION("getCurrentBestResultImage") {
    auto idx = 1;
    auto values = optimizer.getBestResultValues(idx);
    REQUIRE(values.size() == 10000);
    REQUIRE(*std::ranges::max_element(values) > 0.0);
    REQUIRE(*std::ranges::min_element(values) == dbl_approx(0.0));
  }
}
