#include "sme/optimize.hpp"
#include "optimize_impl.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include <iostream>
#include <pagmo/algorithms/pso.hpp>
#include <utility>

namespace sme::simulate {

Optimization::Optimization(sme::model::Model &model)
    : options{model.getOptimizeOptions()} {
  PagmoUDP udp{};
  udp.init(model.getXml().toStdString(), options);
  prob = pagmo::problem{std::move(udp)};
  algo = pagmo::algorithm{pagmo::pso()};
  pop = pagmo::population{prob, options.nParticles};
  bestFitness.push_back(pop.champion_f().front());
  bestParams.push_back(pop.champion_x());
}

std::size_t Optimization::evolve(std::size_t n) {
  if (isRunning) {
    SPDLOG_WARN("Evolve is currently running: ignoring call to evolve");
    return 0;
  }
  stopRequested = false;
  isRunning = true;
  SPDLOG_INFO("Starting {} evolve steps", n);
  // ensure output vectors won't re-allocate during evolution
  bestFitness.reserve(bestFitness.size() + n);
  bestParams.reserve(bestParams.size() + n);
  for (std::size_t i = 0; i < n; ++i) {
    pop = algo.evolve(pop);
    bestFitness.push_back(pop.champion_f().front());
    bestParams.push_back(pop.champion_x());
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
  applyParameters(bestParams.back(), options.optParams, model);
}

const std::vector<std::vector<double>> &Optimization::getParams() const {
  return bestParams;
}

std::vector<QString> Optimization::getParamNames() const {
  std::vector<QString> names;
  names.reserve(options.optParams.size());
  for (const auto &optParam : options.optParams) {
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
