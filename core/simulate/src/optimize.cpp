#include "sme/optimize.hpp"
#include "optimize_impl.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
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

Optimization::Optimization(sme::model::Model &model) {
  const auto &options{model.getOptimizeOptions()};
  xmlModel = std::make_unique<std::string>(model.getXml().toStdString());
  optimizeOptions =
      std::make_unique<OptimizeOptions>(model.getOptimizeOptions());
  optTimesteps = std::make_unique<std::vector<sme::simulate::OptTimestep>>(
      getOptTimesteps(options));
  modelQueue = std::make_unique<
      oneapi::tbb::concurrent_queue<std::shared_ptr<sme::model::Model>>>();
  switch (optimizeOptions->optAlgorithm.optAlgorithmType) {
  case sme::simulate::OptAlgorithmType::PSO:
    algo = std::make_unique<pagmo::algorithm>(pagmo::pso());
  case sme::simulate::OptAlgorithmType::GPSO:
    algo = std::make_unique<pagmo::algorithm>(pagmo::pso_gen());
  case sme::simulate::OptAlgorithmType::DE:
    algo = std::make_unique<pagmo::algorithm>(pagmo::de());
  case sme::simulate::OptAlgorithmType::iDE:
    algo = std::make_unique<pagmo::algorithm>(pagmo::sade(1, 2, 2));
  case sme::simulate::OptAlgorithmType::jDE:
    algo = std::make_unique<pagmo::algorithm>(pagmo::sade(1, 2, 1));
  case sme::simulate::OptAlgorithmType::pDE:
    algo = std::make_unique<pagmo::algorithm>(pagmo::de1220());
  case sme::simulate::OptAlgorithmType::ABC:
    algo = std::make_unique<pagmo::algorithm>(pagmo::bee_colony());
  case sme::simulate::OptAlgorithmType::gaco:
    algo = std::make_unique<pagmo::algorithm>(pagmo::gaco());
  default:
    algo = std::make_unique<pagmo::algorithm>(pagmo::pso());
  }

  // construct models in queue in serial for now to aovid libsbml thread safety
  // issues (see
  // https://github.com/spatial-model-editor/spatial-model-editor/issues/786)
  // todo: once that is fixed, can remove this & let the UDP construct them as
  // needed
  for (std::size_t i = 0; i < optimizeOptions->optAlgorithm.islands; ++i) {
    auto m{std::make_shared<sme::model::Model>()};
    m->importSBMLString(*xmlModel);
    modelQueue->push(std::move(m));
  }
}

std::size_t Optimization::evolve(std::size_t n) {
  if (isRunning) {
    SPDLOG_WARN("Evolve is currently running: ignoring call to evolve");
    return 0;
  }
  stopRequested = false;
  isRunning = true;
  if (archi == nullptr) {
    archi = std::make_unique<pagmo::archipelago>(
        optimizeOptions->optAlgorithm.islands, *algo,
        pagmo::problem{PagmoUDP(xmlModel.get(), optimizeOptions.get(),
                                optTimesteps.get(), modelQueue.get())},
        optimizeOptions->optAlgorithm.population);
    appendBestFitnesssAndParams(*archi, bestFitness, bestParams);
  }
  SPDLOG_INFO("Starting {} evolve steps", n);
  // ensure output vectors won't re-allocate during evolution
  bestFitness.reserve(bestFitness.size() + n);
  bestParams.reserve(bestParams.size() + n);
  for (std::size_t i = 0; i < n; ++i) {
    archi->evolve();
    archi->wait_check();
    appendBestFitnesssAndParams(*archi, bestFitness, bestParams);
    ++nIterations;
    if (stopRequested) {
      SPDLOG_INFO("Stopping evolve early after {} steps", nIterations);
      isRunning = false;
      stopRequested = false;
      return nIterations;
    }
  }
  SPDLOG_INFO("Completed {} steps", nIterations);
  isRunning = false;
  stopRequested = false;
  return nIterations;
}

void Optimization::applyParametersToModel(sme::model::Model *model) const {
  applyParameters(bestParams.back(), model);
}

const std::vector<std::vector<double>> &Optimization::getParams() const {
  return bestParams;
}

std::vector<QString> Optimization::getParamNames() const {
  std::vector<QString> names;
  names.reserve(optimizeOptions->optParams.size());
  for (const auto &optParam : optimizeOptions->optParams) {
    names.push_back(optParam.name.c_str());
  }
  return names;
}

const std::vector<double> &Optimization::getFitness() const {
  return bestFitness;
}

std::size_t Optimization::getIterations() const { return nIterations.load(); };

bool Optimization::getIsRunning() const { return isRunning.load(); }

bool Optimization::getIsStopping() const { return stopRequested.load(); }

void Optimization::requestStop() { stopRequested.store(true); }

} // namespace sme::simulate
