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
#include <pagmo/algorithms/nlopt.hpp>
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
  std::ranges::sort(sortedUniqueTimes);
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
    using enum sme::simulate::OptAlgorithmType;
  case PSO:
    return std::make_unique<pagmo::algorithm>(pagmo::pso());
  case GPSO:
    return std::make_unique<pagmo::algorithm>(pagmo::pso_gen());
  case DE:
    return std::make_unique<pagmo::algorithm>(pagmo::de());
  case iDE:
    return std::make_unique<pagmo::algorithm>(pagmo::sade(1, 2, 2));
  case jDE:
    return std::make_unique<pagmo::algorithm>(pagmo::sade(1, 2, 1));
  case pDE:
    return std::make_unique<pagmo::algorithm>(pagmo::de1220());
  case ABC:
    return std::make_unique<pagmo::algorithm>(pagmo::bee_colony());
  case gaco:
    return std::make_unique<pagmo::algorithm>(pagmo::gaco(1, 7));
  // below are NLopt algorithms.
  // https://esa.github.io/pagmo2/docs/cpp/algorithms/nlopt.html
  case COBYLA: {
    auto algo = pagmo::nlopt("cobyla");
    algo.set_xtol_rel(
        0); // this serves to effectively disable the stopping criterion based
            // on the relative change in the parameters
    algo.set_maxeval(10);
    return std::make_unique<pagmo::algorithm>(std::move(algo));
  }
  case BOBYQA: {
    auto algo = pagmo::nlopt("bobyqa");
    algo.set_xtol_rel(0);
    algo.set_maxeval(10);
    return std::make_unique<pagmo::algorithm>(std::move(algo));
  }
  case NMS: {
    auto algo = pagmo::nlopt("neldermead");
    algo.set_xtol_rel(0);
    algo.set_maxeval(10);
    return std::make_unique<pagmo::algorithm>(std::move(algo));
  }
  case sbplx: {
    auto algo = pagmo::nlopt("sbplx");
    algo.set_xtol_rel(0);
    algo.set_maxeval(10);
    return std::make_unique<pagmo::algorithm>(std::move(algo));
  }
  case AL: {
    // README: check
    // https://esa.github.io/pagmo2/docs/cpp/algorithms/nlopt.html?highlight=nlopt
    // under 'set_local_optimizer' for more info
    auto algo = pagmo::nlopt("auglag");
    auto aux_algo = pagmo::nlopt("neldermead");
    algo.set_xtol_rel(0);
    algo.set_maxeval(10);
    aux_algo.set_xtol_rel(0);
    aux_algo.set_maxeval(10);
    algo.set_local_optimizer(aux_algo);
    return std::make_unique<pagmo::algorithm>(std::move(algo));
  }
  case PRAXIS: {
    auto algo = pagmo::nlopt("praxis");
    algo.set_xtol_rel(0);
    algo.set_maxeval(10);
    return std::make_unique<pagmo::algorithm>(std::move(algo));
  }
  default:
    SPDLOG_INFO("Unknown optimization algorithm: using PSO");
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

  // nlopt algorithms can have population < 2, while the others do not.
  auto nloptAlgorithms = {OptAlgorithmType::COBYLA, OptAlgorithmType::BOBYQA,
                          OptAlgorithmType::NMS,    OptAlgorithmType::sbplx,
                          OptAlgorithmType::AL,     OptAlgorithmType::PRAXIS};

  if (options.optAlgorithm.population < 2 &&
      std::ranges::find(nloptAlgorithms,
                        options.optAlgorithm.optAlgorithmType) ==
          nloptAlgorithms.end()) {
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
  modelQueue = std::make_unique<sme::simulate::ThreadsafeModelQueue>();
  algo = getPagmoAlgorithm(
      optConstData->optimizeOptions.optAlgorithm.optAlgorithmType);

  // README: construct models in queue in serial for now to avoid libsbml thread
  // safety issues (see
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
      SPDLOG_INFO("Stopping evolve early after {} steps", nIterations.load());
      return finalizeEvolve();
    }
  }
  SPDLOG_INFO("Completed {} steps", nIterations.load());
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

sme::common::Volume Optimization::getImageSize() {
  return optConstData->imageSize;
}

double Optimization::getMaxValue(std::size_t index) {
  if (index >= optConstData->maxTargetValues.size()) {
    SPDLOG_CRITICAL("index outside of range for max value retrieval: {} , {}",
                    index, optConstData->maxTargetValues.size());
    return 0;
  }
  return optConstData->maxTargetValues[index];
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

common::ImageStack Optimization::getDifferenceImage(std::size_t index) {
  // SPDLOG_INFO("getDifferenceImage({})", index);

  auto size = getImageSize();
  // SPDLOG_INFO("volume: ({}, {}, {}), index: {}", size.width(), size.height(),
  // size.depth(), index);
  auto tgt_values = getTargetValues(index);

  // separate allocation to make sure that common::max does not segfault when
  // called on an empty array.
  auto diff_values =
      std::vector<double>(size.width() * size.height() * size.depth(), 0);
  auto res_values = getBestResultValues(index);
  // SPDLOG_INFO("target values: {}", tgt_values.size());
  // SPDLOG_INFO("result values: {}", res_values.size());
  if (res_values.size() > 0) {
    // SPDLOG_INFO("diff values max before: {}", common::max(diff_values));
    // SPDLOG_INFO("tgt_values values max: {}", common::max(tgt_values));
    // SPDLOG_INFO("res_values values max: {}", common::max(res_values));
    std::ranges::transform(tgt_values, res_values, diff_values.begin(),
                           std::minus<double>());
  }
  // SPDLOG_INFO("diff values: {}", diff_values.size());
  // SPDLOG_INFO("max diff value: {}", common::max(diff_values));
  return sme::common::ImageStack(size, diff_values, common::max(diff_values));
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
  SPDLOG_INFO("getUpdatedBestResultImage({})", index);
  SPDLOG_INFO("bestResults.imageIndex: {}", bestResults.imageIndex);
  if (bestResults.values.empty()) {
    SPDLOG_INFO("bestResults.values is empty");
    return {};
  }
  if (bestResults.imageChanged || index != bestResults.imageIndex) {
    SPDLOG_INFO("bestResults.values is not empty");
    SPDLOG_INFO("bestResults size: {}, max {}",
                bestResults.values[index].size(),
                common::max(bestResults.values[index]));

    bestResults.imageChanged = false;
    bestResults.imageIndex = index;
    return common::ImageStack(optConstData->imageSize,
                              bestResults.values[index],
                              optConstData->maxTargetValues[index]);
  } else {
    SPDLOG_INFO("bestResults.imageChanged is false and index is the same");
  }
  return {};
}

common::ImageStack Optimization::getCurrentBestResultImage() const {
  std::scoped_lock lock{bestResultsMutex};
  return common::ImageStack(
      optConstData->imageSize, bestResults.values[bestResults.imageIndex],
      optConstData->maxTargetValues[bestResults.imageIndex]);
}

std::vector<double> Optimization::getBestResultValues(std::size_t index) const {
  std::scoped_lock lock{
      bestResultsMutex}; // TODO: lock to avoid getting while it's still set?
                         // not sure it's necessary, but will leave it here just
                         // to be sure
  SPDLOG_INFO("getBestResultValues({})", index);
  SPDLOG_INFO("bestResults.values.size(): {}", bestResults.values.size());
  if (index > bestResults.values.size() or bestResults.values.empty()) {
    SPDLOG_CRITICAL("index outside of range for result retrieval: {} , {}",
                    index, bestResults.values.size());
    return {};
  }
  return bestResults.values[index];
}

std::vector<double> Optimization::getTargetValues(std::size_t index) const {

  if (index > optConstData->optimizeOptions.optCosts.size()) {
    SPDLOG_CRITICAL("index outside of range for target retrieval: {} , {}",
                    index, bestResults.values.size());
    return {};
  }

  return optConstData->optimizeOptions.optCosts[index].targetValues;
}

std::size_t Optimization::getIterations() const { return nIterations.load(); };

bool Optimization::getIsRunning() const { return isRunning.load(); }

bool Optimization::getIsStopping() const { return stopRequested.load(); }

void Optimization::requestStop() { stopRequested.store(true); }

const std::string &Optimization::getErrorMessage() const {
  return errorMessage;
}

} // namespace sme::simulate
