#include "sme/optimize.hpp"
#include "optimize_impl.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <iostream>
#include <pagmo/algorithms/bee_colony.hpp>
#include <pagmo/algorithms/de.hpp>
#include <pagmo/algorithms/de1220.hpp>
#include <pagmo/algorithms/gaco.hpp>
#include <pagmo/algorithms/pso.hpp>
#include <pagmo/algorithms/pso_gen.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <utility>

namespace sme::simulate {

static void
appendBestFitnesssAndParams(const pagmo::archipelago &archipelago,
                            std::vector<double> &bestFitness,
                            std::vector<std::vector<double>> &bestParams) {
  auto fitness{std::numeric_limits<double>::max()};
  std::vector<double> params;
  for (const auto &island : archipelago) {
    auto pop{island.get_population()};
    if (auto f{pop.champion_f()[0]}; f < fitness) {
      fitness = f;
      params = pop.champion_x();
    }
  }
  bestFitness.push_back(fitness);
  bestParams.push_back(std::move(params));
}

static std::vector<OptTimestep>
getOptTimesteps(const OptimizeOptions &options) {
  std::vector<OptTimestep> optTimesteps;
  // get time for each optCost
  std::vector<double> times;
  for (const auto &optCost : options.optCosts) {
    times.push_back(optCost.simulationTime);
  }
  // sort times
  std::vector sortedUniqueTimes{times};
  std::sort(sortedUniqueTimes.begin(), sortedUniqueTimes.end());
  // margin within which times are considered equal:
  constexpr double relativeEps{1e-13};
  double epsilon{sortedUniqueTimes.back() * relativeEps};
  // remove (approx) duplicates
  sortedUniqueTimes.erase(std::unique(sortedUniqueTimes.begin(),
                                      sortedUniqueTimes.end(),
                                      [epsilon](double a, double b) {
                                        return std::abs(a - b) < epsilon;
                                      }),
                          sortedUniqueTimes.end());
  double previousTime{0};
  for (double sortedUniqueTime : sortedUniqueTimes) {
    double dt{sortedUniqueTime - previousTime};
    previousTime += dt;
    optTimesteps.push_back({dt, {}});
    for (std::size_t i = 0; i < times.size(); ++i) {
      if (std::abs(times[i] - sortedUniqueTime) < epsilon) {
        optTimesteps.back().optCostIndices.push_back(i);
      }
    }
  }
  for (const auto &optTimestep : optTimesteps) {
    SPDLOG_INFO("t = {}", optTimestep.simulationTime);
    for (const auto optCostIndex : optTimestep.optCostIndices) {
      SPDLOG_INFO("  - index {}", optCostIndex);
    }
  }
  return optTimesteps;
}

static std::unique_ptr<pagmo::algorithm>
getPagmoAlgorithm(sme::simulate::OptAlgorithmType optAlgorithmType) {
  // https://esa.github.io/pagmo2/docs/cpp/cpp_docs.html#implemented-algorithms
  switch (optAlgorithmType) {
  case sme::simulate::OptAlgorithmType::PSO:
    return std::make_unique<pagmo::algorithm>(pagmo::pso());
  case sme::simulate::OptAlgorithmType::GPSO:
    return std::make_unique<pagmo::algorithm>(pagmo::pso_gen());
  case sme::simulate::OptAlgorithmType::DE:
    return std::make_unique<pagmo::algorithm>(pagmo::de());
  case sme::simulate::OptAlgorithmType::iDE:
    return std::make_unique<pagmo::algorithm>(pagmo::sade(1, 2, 2));
  case sme::simulate::OptAlgorithmType::jDE:
    return std::make_unique<pagmo::algorithm>(pagmo::sade(1, 2, 1));
  case sme::simulate::OptAlgorithmType::pDE:
    return std::make_unique<pagmo::algorithm>(pagmo::de1220());
  case sme::simulate::OptAlgorithmType::ABC:
    return std::make_unique<pagmo::algorithm>(pagmo::bee_colony());
  case sme::simulate::OptAlgorithmType::gaco:
    return std::make_unique<pagmo::algorithm>(pagmo::gaco(1, 7));
  default:
    return std::make_unique<pagmo::algorithm>(pagmo::pso());
  }
}

std::size_t Optimization::finalizeEvolve(const std::string &newErrorMessage) {
  if (!newErrorMessage.empty()) {
    errorMessage = newErrorMessage;
  }
  if (!errorMessage.empty()) {
    SPDLOG_ERROR("{}", errorMessage);
  }
  isRunning.store(false);
  stopRequested.store(false);
  return nIterations;
}

Optimization::Optimization(sme::model::Model &model) {
  const auto &options{model.getOptimizeOptions()};
  if (options.optAlgorithm.population < 2) {
    errorMessage = "Invalid optimization population size, can't be less than 2";
    return;
  }
  optConstData = std::make_unique<sme::simulate::OptConstData>();
  optConstData->imageSize = model.getGeometry().getImages().volume();
  optConstData->xmlModel = model.getXml().toStdString();
  optConstData->optimizeOptions = model.getOptimizeOptions();
  optConstData->optTimesteps = getOptTimesteps(options);
  for (const auto &cost : model.getOptimizeOptions().optCosts) {
    if (cost.targetValues.empty()) {
      // empty vector is implicitly zero everywhere,
      // use negative value here to allow rescaling of image to whatever the
      // result is
      optConstData->maxTargetValues.push_back(-1.0);
    } else {
      optConstData->maxTargetValues.push_back(
          sme::common::max(cost.targetValues));
    }
  }
  modelQueue = std::make_unique<
      oneapi::tbb::concurrent_queue<std::shared_ptr<sme::model::Model>>>();
  algo = getPagmoAlgorithm(
      optConstData->optimizeOptions.optAlgorithm.optAlgorithmType);

  // construct models in queue in serial for now to avoid libsbml thread safety
  // issues (see
  // https://github.com/spatial-model-editor/spatial-model-editor/issues/786)
  // todo: once that is fixed, can remove this & let the UDP construct them as
  // needed
  for (std::size_t i = 0;
       i < optConstData->optimizeOptions.optAlgorithm.islands; ++i) {
    auto m{std::make_shared<sme::model::Model>()};
    m->importSBMLString(optConstData->xmlModel);
    modelQueue->push(std::move(m));
  }
}

std::size_t Optimization::evolve(std::size_t n) {
  if (isRunning.load()) {
    SPDLOG_WARN("Evolve is currently running: ignoring call to evolve");
    return 0;
  }
  if (!errorMessage.empty()) {
    return finalizeEvolve();
  }
  errorMessage.clear();
  stopRequested.store(false);
  isRunning.store(true);
  if (archi == nullptr) {
    try {
      archi = std::make_unique<pagmo::archipelago>(
          optConstData->optimizeOptions.optAlgorithm.islands, *algo,
          pagmo::problem{PagmoUDP(optConstData.get(), modelQueue.get(), this)},
          optConstData->optimizeOptions.optAlgorithm.population);
    } catch (const std::invalid_argument &e) {
      return finalizeEvolve(e.what());
    }
    appendBestFitnesssAndParams(*archi, bestFitness, bestParams);
  }
  SPDLOG_INFO("Starting {} {} evolve steps", n, algo->get_name());
  // ensure output vectors won't re-allocate during evolution
  bestFitness.reserve(bestFitness.size() + n);
  bestParams.reserve(bestParams.size() + n);
  for (std::size_t i = 0; i < n; ++i) {
    try {
      archi->evolve();
      archi->wait_check();
    } catch (const std::invalid_argument &e) {
      return finalizeEvolve(e.what());
    }
    appendBestFitnesssAndParams(*archi, bestFitness, bestParams);
    ++nIterations;
    if (stopRequested) {
      SPDLOG_INFO("Stopping evolve early after {} steps", nIterations);
      return finalizeEvolve();
    }
  }
  SPDLOG_INFO("Completed {} steps", nIterations);
  return finalizeEvolve();
}

bool Optimization::applyParametersToModel(sme::model::Model *model) const {
  if (bestParams.empty()) {
    return false;
  }
  applyParameters(bestParams.back(), model);
  return true;
}

const std::vector<std::vector<double>> &Optimization::getParams() const {
  return bestParams;
}

std::vector<QString> Optimization::getParamNames() const {
  std::vector<QString> names;
  names.reserve(optConstData->optimizeOptions.optParams.size());
  for (const auto &optParam : optConstData->optimizeOptions.optParams) {
    names.push_back(optParam.name.c_str());
  }
  return names;
}

const std::vector<double> &Optimization::getFitness() const {
  return bestFitness;
}

bool Optimization::setBestResults(double fitness,
                                  std::vector<std::vector<double>> &&results) {
  std::scoped_lock lock{bestResultsMutex};
  if (fitness < bestResults.fitness) {
    bestResults.values = std::move(results);
    bestResults.fitness = fitness;
    bestResults.imageChanged = true;
    return true;
  }
  return false;
}

common::ImageStack Optimization::getTargetImage(std::size_t index) const {
  return common::ImageStack(
      optConstData->imageSize,
      optConstData->optimizeOptions.optCosts[index].targetValues);
}

std::optional<common::ImageStack>
Optimization::getUpdatedBestResultImage(std::size_t index) {
  std::scoped_lock lock{bestResultsMutex};
  if (bestResults.values.empty()) {
    return {};
  }
  if (bestResults.imageChanged || index != bestResults.imageIndex) {
    bestResults.imageChanged = false;
    bestResults.imageIndex = index;
    return common::ImageStack(optConstData->imageSize,
                              bestResults.values[index],
                              optConstData->maxTargetValues[index]);
  }
  return {};
}

std::size_t Optimization::getIterations() const { return nIterations.load(); };

bool Optimization::getIsRunning() const { return isRunning.load(); }

bool Optimization::getIsStopping() const { return stopRequested.load(); }

void Optimization::requestStop() { stopRequested.store(true); }

const std::string &Optimization::getErrorMessage() const {
  return errorMessage;
}

} // namespace sme::simulate
