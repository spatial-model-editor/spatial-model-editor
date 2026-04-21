#include "pixelsim.hpp"
#include "pixelsim_common.hpp"
#include "pixelsim_impl.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/simple_symbolic.hpp"
#include "sme/utils.hpp"
#include <QElapsedTimer>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>
#include <utility>

namespace sme::simulate {

void PixelSim::solveZeroStorageConstraints() {
  if (!hasAnyZeroStorageSpecies) {
    return;
  }
  constexpr std::size_t maxIterations{100};
  double nextRelaxDt = maxRelaxStableTimestep;
  auto computeResidual = [this]() {
    for (auto &sim : simCompartments) {
      if (useTBB) {
        sim->evaluateReactionsAndDiffusion_tbb();
      } else {
        sim->evaluateReactionsAndDiffusion();
      }
    }
    for (auto &sim : simMembranes) {
      sim->evaluateReactions();
    }
    for (auto &sim : simCompartments) {
      sim->spatiallyAverageDcdt();
    }
  };
  for (std::size_t iter = 0; iter < maxIterations; ++iter) {
    // stage 1: evaluate residual
    computeResidual();
    // check convergence (both absolute and relative residual)
    PixelIntegratorError residual{0.0, 0.0};
    for (const auto &sim : simCompartments) {
      auto compRes = sim->getZeroStorageResidual(epsilon);
      residual.abs = std::max(residual.abs, compRes.abs);
      residual.rel = std::max(residual.rel, compRes.rel);
    }
    if (residual.abs < errMax.abs && residual.rel < errMax.rel) {
      return;
    }
    // adaptive RK212 step
    double dt = nextRelaxDt;
    // substep 1: save old, c += dt * dcdt for zero-storage species
    for (auto &sim : simCompartments) {
      sim->doRelaxSubstep1(dt);
    }
    // stage 2: evaluate residual at updated state
    computeResidual();
    // substep 2: c = 0.5*old + 0.5*c + 0.5*dt*dcdt (second-order)
    for (auto &sim : simCompartments) {
      sim->doRelaxSubstep2(dt);
    }
    // error estimate and adaptive timestep
    PixelIntegratorError err{0.0, 0.0};
    for (const auto &sim : simCompartments) {
      auto compErr = sim->calculateRelaxError(epsilon);
      err.rel = std::max(err.rel, compErr.rel);
      err.abs = std::max(err.abs, compErr.abs);
    }
    double errFactor = std::min(errMax.abs / (err.abs + 1e-300),
                                errMax.rel / (err.rel + 1e-300));
    errFactor = std::sqrt(errFactor); // RK2 order
    nextRelaxDt = std::min(0.95 * dt * errFactor, maxRelaxStableTimestep);
    if (err.abs > errMax.abs || err.rel > errMax.rel) {
      // error too large: undo step and retry with smaller dt
      for (auto &sim : simCompartments) {
        sim->undoRelaxStep();
      }
    }
  }
  SPDLOG_WARN("Zero-storage constraint solver did not converge after {} "
              "iterations",
              maxIterations);
}

void PixelSim::calculateDcdt() {
  // calculate dcd/dt in all compartments
  maxStableTimestep = std::numeric_limits<double>::max();
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->evaluateReactionsAndDiffusion_tbb();
    } else {
      sim->evaluateReactionsAndDiffusion();
    }
    maxStableTimestep =
        std::min(maxStableTimestep, sim->getMaxStableTimestep());
  }
  // membrane contribution to dc/dt
  for (auto &sim : simMembranes) {
    if (useTBB) {
      sim->evaluateReactions_tbb();
    } else {
      sim->evaluateReactions();
    }
  }
  for (auto &sim : simCompartments) {
    sim->spatiallyAverageDcdt();
    if (useTBB) {
      sim->applyStorage_tbb();
    } else {
      sim->applyStorage();
    }
  }
}

double PixelSim::doRK101(double dt) {
  // RK1(0)1: Forwards Euler, no error estimate
  solveZeroStorageConstraints();
  calculateDcdt();
  dt = std::min(dt, maxStableTimestep);
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->doForwardsEulerTimestep_tbb(dt);
    } else {
      sim->doForwardsEulerTimestep(dt);
    }
    if (useTBB) {
      sim->clampNegativeConcentrations_tbb();
    } else {
      sim->clampNegativeConcentrations();
    }
  }
  return dt;
}

void PixelSim::doRK212(double dt) {
  // RK2(1)2: Heun / Modified Euler, with embedded forwards Euler error
  // estimate Shu-Osher form used here taken from eq(2.15) of
  // https://doi.org/10.1016/0021-9991(88)90177-5
  solveZeroStorageConstraints();
  calculateDcdt();
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->doRK212Substep1_tbb(dt);
    } else {
      sim->doRK212Substep1(dt);
    }
  }
  solveZeroStorageConstraints();
  calculateDcdt();
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->doRK212Substep2_tbb(dt);
    } else {
      sim->doRK212Substep2(dt);
    }
  }
}

void PixelSim::doRK323(double dt) {
  using namespace detail::rk323;
  for (auto &sim : simCompartments) {
    sim->doRKInit();
  }
  for (std::size_t i = 0; i < g1.size(); ++i) {
    doRKSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
  }
  for (auto &sim : simCompartments) {
    sim->doRKFinalise(finaliseFactors[0], finaliseFactors[1],
                      finaliseFactors[2]);
  }
}

void PixelSim::doRK435(double dt) {
  using namespace detail::rk435;
  for (auto &sim : simCompartments) {
    sim->doRKInit();
  }
  for (std::size_t i = 0; i < g1.size(); ++i) {
    doRKSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
  }
  for (auto &sim : simCompartments) {
    sim->doRKFinalise(deltaSumReciprocal * delta[5], deltaSumReciprocal,
                      deltaSumReciprocal * delta[6]);
  }
}

void PixelSim::doRKSubstep(double dt, double g1, double g2, double g3,
                           double beta, double delta) {
  solveZeroStorageConstraints();
  calculateDcdt();
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->doRKSubstep_tbb(dt, g1, g2, g3, beta, delta);
    } else {
      sim->doRKSubstep(dt, g1, g2, g3, beta, delta);
    }
  }
}

double PixelSim::doRKAdaptive(double dtMax) {
  // Adaptive timestep Runge-Kutta
  PixelIntegratorError err;
  double dt;
  double errPower = detail::getErrorPower(integrator);
  do {
    // do timestep
    dt = std::min(nextTimestep, dtMax);
    if (integrator == PixelIntegratorType::RK212) {
      doRK212(dt);
    } else if (integrator == PixelIntegratorType::RK323) {
      doRK323(dt);
    } else if (integrator == PixelIntegratorType::RK435) {
      doRK435(dt);
    }
    // calculate error
    err.abs = 0;
    err.rel = 0;
    for (const auto &sim : simCompartments) {
      auto compErr = sim->calculateRKError(epsilon);
      err.rel = std::max(err.rel, compErr.rel);
      err.abs = std::max(err.abs, compErr.abs);
    }
    // calculate new timestep
    double errFactor = std::min(errMax.abs / err.abs, errMax.rel / err.rel);
    errFactor = std::pow(errFactor, errPower);
    nextTimestep = std::min(0.95 * dt * errFactor, dtMax);
    SPDLOG_TRACE("dt = {} gave rel err = {}, abs err = {} -> new dt = {}", dt,
                 err.rel, err.abs, nextTimestep);
    if (nextTimestep / dtMax < 1e-20) {
      currentErrorImages.clear();
      std::string problemSpecies{"unknown"};
      for (const auto &sim : simCompartments) {
        auto speciesName{
            sim->plotRKError(currentErrorImages, epsilon, err.rel)};
        if (!speciesName.empty()) {
          problemSpecies = speciesName;
        }
      }
      currentErrorMessage = fmt::format(
          "Failed to solve model to required accuracy. The largest "
          "relative integration error comes from species '{}'. The location "
          "of the pixels with the largest relative integration error are shown "
          "below in red:",
          problemSpecies);
      return nextTimestep;
    }
    if (err.abs > errMax.abs || err.rel > errMax.rel) {
      SPDLOG_TRACE("discarding step");
      ++discardedSteps;
      for (auto &sim : simCompartments) {
        sim->undoRKStep();
      }
    }
  } while (err.abs > errMax.abs || err.rel > errMax.rel);
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->clampNegativeConcentrations_tbb();
    } else {
      sim->clampNegativeConcentrations();
    }
  }
  return dt;
}

PixelSim::PixelSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    const std::map<std::string, double, std::less<>> &substitutions)
    : PixelSimBase{sbmlDoc.getSimulationSettings().options.pixel.integrator,
                   sbmlDoc.getSimulationSettings().options.pixel.maxErr,
                   sbmlDoc.getSimulationSettings().options.pixel.maxTimestep},
      doc{sbmlDoc},
      numMaxThreads{sbmlDoc.getSimulationSettings().options.pixel.maxThreads} {
  try {
    // check if reactions explicitly depend on time or space
    auto xId{doc.getParameters().getSpatialCoordinates().x.id};
    auto yId{doc.getParameters().getSpatialCoordinates().y.id};
    auto zId{doc.getParameters().getSpatialCoordinates().z.id};
    const auto crossDiffusionDependsOnVariable =
        [this, &compartmentSpeciesIds](const std::string &variableId) {
          for (const auto &speciesIds : compartmentSpeciesIds) {
            for (const auto &targetSpeciesId : speciesIds) {
              for (const auto &sourceSpeciesId : speciesIds) {
                if (targetSpeciesId == sourceSpeciesId) {
                  continue;
                }
                const auto expression =
                    doc.getSpecies().getCrossDiffusionConstant(
                        targetSpeciesId.c_str(), sourceSpeciesId.c_str());
                if (expression.isEmpty()) {
                  continue;
                }
                if (common::SimpleSymbolic::contains(expression.toStdString(),
                                                     variableId)) {
                  return true;
                }
              }
            }
          }
          return false;
        };
    bool timeDependent{doc.getReactions().dependOnVariable("time") ||
                       crossDiffusionDependsOnVariable("time")};
    if (timeDependent) {
      ++nExtraVars;
    }
    bool spaceDependent{doc.getReactions().dependOnVariable(xId.c_str()) ||
                        doc.getReactions().dependOnVariable(yId.c_str()) ||
                        doc.getReactions().dependOnVariable(zId.c_str()) ||
                        crossDiffusionDependsOnVariable(xId) ||
                        crossDiffusionDependsOnVariable(yId) ||
                        crossDiffusionDependsOnVariable(zId)};
    if (spaceDependent) {
      nExtraVars += 3;
    }
    const bool allUniformDiffusion = [this, &compartmentSpeciesIds]() {
      for (const auto &speciesIds : compartmentSpeciesIds) {
        for (const auto &speciesId : speciesIds) {
          if (doc.getSpecies().getIsConstant(speciesId.c_str())) {
            continue;
          }
          const auto *field = doc.getSpecies().getField(speciesId.c_str());
          if (field == nullptr || !field->getIsUniformDiffusionConstant()) {
            return false;
          }
        }
      }
      return true;
    }();
    if (allUniformDiffusion) {
      SPDLOG_INFO(
          "Pixel solver: using uniform diffusion operator (all species have "
          "uniform diffusion constants)");
    }
    // add compartments
    for (std::size_t compIndex = 0; compIndex < compartmentIds.size();
         ++compIndex) {
      const auto &speciesIds{compartmentSpeciesIds[compIndex]};
      const auto *compartment{doc.getCompartments().getCompartment(
          compartmentIds[compIndex].c_str())};
      simCompartments.push_back(std::make_unique<SimCompartment>(
          doc, compartment, speciesIds,
          sbmlDoc.getSimulationSettings().options.pixel.doCSE,
          sbmlDoc.getSimulationSettings().options.pixel.optLevel, timeDependent,
          spaceDependent, allUniformDiffusion, substitutions));
      maxStableTimestep = std::min(
          maxStableTimestep, simCompartments.back()->getMaxStableTimestep());
      if (simCompartments.back()->getHasZeroStorageSpecies()) {
        hasAnyZeroStorageSpecies = true;
        maxRelaxStableTimestep =
            std::min(maxRelaxStableTimestep,
                     simCompartments.back()->getMaxRelaxStableTimestep());
      }
    }
    if (hasAnyZeroStorageSpecies &&
        maxRelaxStableTimestep == std::numeric_limits<double>::max()) {
      // all zero-storage species have D=0: use fallback dt
      maxRelaxStableTimestep = 1.0;
    }
    // add membranes
    for (const auto &membrane : doc.getMembranes().getMembranes()) {
      if (auto reacsInMembrane =
              doc.getReactions().getIds(membrane.getId().c_str());
          !reacsInMembrane.isEmpty()) {
        // look for the two membrane compartments in simCompartments
        std::string compIdA = membrane.getCompartmentA()->getId();
        std::string compIdB = membrane.getCompartmentB()->getId();
        auto iterA =
            std::ranges::find_if(simCompartments, [&compIdA](const auto &c) {
              return c->getCompartmentId() == compIdA;
            });
        auto iterB =
            std::ranges::find_if(simCompartments, [&compIdB](const auto &c) {
              return c->getCompartmentId() == compIdB;
            });
        SimCompartment *compA{nullptr};
        if (iterA != simCompartments.cend()) {
          compA = iterA->get();
        }
        SimCompartment *compB{nullptr};
        if (iterB != simCompartments.cend()) {
          compB = iterB->get();
        }
        simMembranes.push_back(std::make_unique<SimMembrane>(
            doc, &membrane, compA, compB,
            sbmlDoc.getSimulationSettings().options.pixel.doCSE,
            sbmlDoc.getSimulationSettings().options.pixel.optLevel,
            timeDependent, spaceDependent, substitutions));
      }
    }
    // apply existing simulation concentrations if present
    const auto &data{sbmlDoc.getSimulationData()};
    if (data.concentration.size() > 1 && !data.concentration.back().empty() &&
        (data.concentration.back().size() == simCompartments.size())) {
      SPDLOG_INFO("Applying supplied initial concentrations");
      for (std::size_t i = 0; i < simCompartments.size(); ++i) {
        simCompartments[i]->setConcentrations(data.concentration.back()[i]);
      }
    }
    if (sbmlDoc.getSimulationSettings().options.pixel.enableMultiThreading) {
      useTBB = true;
    }
    if (numMaxThreads == 0) {
      // 0 means use all available threads
      numMaxThreads =
          static_cast<std::size_t>(oneapi::tbb::info::default_concurrency());
    }
  } catch (const std::runtime_error &e) {
    SPDLOG_ERROR("runtime_error: {}", e.what());
    currentErrorMessage = e.what();
  }
}

PixelSim::~PixelSim() = default;

std::size_t PixelSim::run(double time, double timeout_ms,
                          const std::function<bool()> &stopRunningCallback) {
  SPDLOG_TRACE("  - max rel local err {}", errMax.rel);
  SPDLOG_TRACE("  - max abs local err {}", errMax.abs);
  SPDLOG_TRACE("  - max stepsize {}", maxTimestep);
  currentErrorMessage.clear();
  oneapi::tbb::global_control control(
      oneapi::tbb::global_control::max_allowed_parallelism, numMaxThreads);
  QElapsedTimer timer;
  timer.start();
  double tNow = 0;
  std::size_t steps = 0;
  discardedSteps = 0;
  // do timesteps until we reach t
  constexpr double relativeTolerance = 1e-12;
  while (tNow + time * relativeTolerance < time) {
    double maxDt = std::min(maxTimestep, time - tNow);
    if (integrator == PixelIntegratorType::RK101) {
      double timestep = std::min(maxDt, maxStableTimestep);
      timestep = doRK101(timestep);
      tNow += timestep;
    } else {
      tNow += doRKAdaptive(maxDt);
      if (!currentErrorMessage.empty()) {
        return steps;
      }
    }
    ++steps;
    if (timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      setStopRequested(true);
    }
    if (stopRunningCallback && stopRunningCallback()) {
      setStopRequested(true);
      SPDLOG_DEBUG("Simulation cancelled: requesting stop");
    }
    if (stopRequested.load()) {
      currentErrorMessage = "Simulation stopped early";
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      return steps;
    }
  }
  SPDLOG_DEBUG("t={} integrated using {} steps ({:3.1f}% discarded)", time,
               steps + discardedSteps,
               static_cast<double>(100 * discardedSteps) /
                   static_cast<double>(steps + discardedSteps));
  return steps;
}

const std::vector<double> &
PixelSim::getConcentrations(std::size_t compartmentIndex) const {
  return simCompartments[compartmentIndex]->getConcentrations();
}

std::size_t PixelSim::getConcentrationPadding() const { return nExtraVars; }

const std::vector<double> &
PixelSim::getDcdt(std::size_t compartmentIndex) const {
  return simCompartments[compartmentIndex]->getDcdt();
}

double PixelSim::getLowerOrderConcentration(std::size_t compartmentIndex,
                                            std::size_t speciesIndex,
                                            std::size_t pixelIndex) const {
  return simCompartments[compartmentIndex]->getLowerOrderConcentration(
      speciesIndex, pixelIndex);
}

} // namespace sme::simulate
