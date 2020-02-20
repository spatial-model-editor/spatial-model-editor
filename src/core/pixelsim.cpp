#include "pixelsim.hpp"

#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
#include <tbb/parallel_for.h>
#endif
#include "geometry.hpp"
#include "logger.hpp"
#include "pde.hpp"
#include "sbml.hpp"
#include "utils.hpp"

namespace sim {

ReacEval::ReacEval(const sbml::SbmlDocWrapper &doc,
                   const std::vector<std::string> &speciesIDs,
                   const std::vector<std::string> &reactionIDs,
                   const std::vector<std::string> &reactionScaleFactors)
    : result(speciesIDs.size(), 0.0), nSpecies(speciesIDs.size()) {
  // construct reaction expressions and stoich matrix
  pde::PDE pde(&doc, speciesIDs, reactionIDs, {}, reactionScaleFactors);
  // compile all expressions with symengine
  sym = symbolic::Symbolic(pde.getRHS(), speciesIDs);
}

void ReacEval::evaluate(double *output, const double *input) const {
  sym.eval(output, input);
}

void SimCompartment::spatiallyAverageDcdt() {
  // for any non-spatial species: spatially average dc/dt:
  // roughly equivalent to infinite rate of diffusion
  std::size_t nSpecies = speciesIds.size();
  for (std::size_t is : nonSpatialSpeciesIndices) {
    double av = 0;
    for (std::size_t ix = 0; ix < comp.nPixels(); ++ix) {
      av += dcdt[ix * nSpecies + is];
    }
    av /= static_cast<double>(comp.nPixels());
    for (std::size_t ix = 0; ix < comp.nPixels(); ++ix) {
      dcdt[ix * nSpecies + is] = av;
    }
  }
}

SimCompartment::SimCompartment(const sbml::SbmlDocWrapper &doc,
                               const geometry::Compartment &compartment)
    : comp(compartment), compartmentId(compartment.getId()) {
  // get species in compartment
  SPDLOG_DEBUG("compartment: {}", compartmentId);
  std::vector<const geometry::Field *> fields;
  speciesIds.clear();
  for (const auto &s : doc.species.at(compartmentId.c_str())) {
    if (!doc.getIsSpeciesConstant(s.toStdString())) {
      speciesIds.push_back(s.toStdString());
      const auto *field = &doc.mapSpeciesIdToField.at(s);
      double pixelWidth = comp.getPixelWidth();
      diffConstants.push_back(field->diffusionConstant / pixelWidth /
                              pixelWidth);
      // forwards euler stability bound: dt < a^2/4D
      maxStableTimestep =
          std::min(maxStableTimestep, 1.0 / (4.0 * diffConstants.back()));
      fields.push_back(field);
      if (!field->isSpatial) {
        nonSpatialSpeciesIndices.push_back(fields.size() - 1);
      }
      SPDLOG_DEBUG("  - adding species: {}", s.toStdString());
    }
  }
  // get reactions in compartment
  std::vector<std::string> reactionIDs;
  if (const auto iter = doc.reactions.find(compartmentId.c_str());
      iter != doc.reactions.cend()) {
    reactionIDs = utils::toStdString(iter->second);
  }
  reacEval = ReacEval(doc, speciesIds, reactionIDs, {});
  // setup concentrations vector with initial values
  conc.resize(fields.size() * compartment.nPixels());
  dcdt.resize(conc.size(), 0.0);
  auto concIter = conc.begin();
  for (std::size_t ix = 0; ix < compartment.nPixels(); ++ix) {
    for (const auto *field : fields) {
      *concIter = field->conc[ix];
      ++concIter;
    }
  }
  assert(concIter == conc.end());
}

void SimCompartment::evaluateDiffusionOperator() {
  std::size_t ns = speciesIds.size();
  std::size_t np = comp.nPixels();
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(
      std::size_t{0}, np,
      [ns, &dcdt = dcdt, &diffConstants = diffConstants, &conc = conc,
       &comp = comp](std::size_t i) {
#else
  for (std::size_t i = 0; i < np; ++i) {
#endif
        std::size_t ix = i * ns;
        std::size_t ix_upx = comp.up_x(i) * ns;
        std::size_t ix_dnx = comp.dn_x(i) * ns;
        std::size_t ix_upy = comp.up_y(i) * ns;
        std::size_t ix_dny = comp.dn_y(i) * ns;
        for (std::size_t is = 0; is < ns; ++is) {
          dcdt[ix + is] +=
              diffConstants[is] *
              (conc[ix_upx + is] + conc[ix_dnx + is] + conc[ix_upy + is] +
               conc[ix_dny + is] - 4.0 * conc[ix + is]);
        }
      }
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  );
#endif
}

void SimCompartment::evaluateReactions() {
  std::size_t ns = speciesIds.size();
  std::size_t N = ns * comp.nPixels();
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(
      std::size_t{0}, N, ns,
      [&reacEval = reacEval, &conc = conc, &dcdt = dcdt](std::size_t index) {
#else
  for (std::size_t index = 0; index < N; index += ns) {
#endif
        reacEval.evaluate(dcdt.data() + index, conc.data() + index);
      }
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  );
#endif
}

void SimCompartment::doForwardsEulerTimestep(double dt) {
  // do forwards euler timestep
  for (std::size_t i = 0; i < conc.size(); ++i) {
    conc[i] += dt * dcdt[i];
  }
}

void SimCompartment::doRKInit() {
  s2.assign(conc.size(), 0.0);
  s3 = conc;
}

void SimCompartment::doRKSubstep(double dt, double g1, double g2, double g3,
                                 double beta, double delta) {
  for (std::size_t i = 0; i < conc.size(); ++i) {
    s2[i] += delta * conc[i];
    conc[i] = g1 * conc[i] + g2 * s2[i] + g3 * s3[i] + beta * dt * dcdt[i];
  }
}

void SimCompartment::doRKFinalise(double cFactor, double s2Factor,
                                  double s3Factor) {
  for (std::size_t i = 0; i < conc.size(); ++i) {
    s2[i] = cFactor * conc[i] + s2Factor * s2[i] + s3Factor * s3[i];
  }
}

void SimCompartment::undoRKStep() {
  for (std::size_t i = 0; i < conc.size(); ++i) {
    conc[i] = s3[i];
  }
}

IntegratorError SimCompartment::calculateRKError(double epsilon) const {
  IntegratorError err;
  for (std::size_t i = 0; i < conc.size(); ++i) {
    double localErr = std::abs(conc[i] - s2[i]);
    err.abs = std::max(err.abs, localErr);
    // average current and previous concentrations and add a (hopefully) small
    // constant term to avoid dividing by c=0 issues
    double localNorm = 0.5 * (conc[i] + s3[i] + epsilon);
    err.rel = std::max(err.rel, localErr / localNorm);
  }
  return err;
}

const std::string &SimCompartment::getCompartmentId() const {
  return compartmentId;
}

const std::vector<std::string> &SimCompartment::getSpeciesIds() const {
  return speciesIds;
}

const std::vector<double> &SimCompartment::getConcentrations() const {
  return conc;
}

double SimCompartment::getLowerOrderConcentration(
    std::size_t speciesIndex, std::size_t pixelIndex) const {
  if (s2.empty()) {
    return 0;
  }
  std::size_t nSpecies = speciesIds.size();
  return s2[pixelIndex * nSpecies + speciesIndex];
}

const std::vector<QPoint> &SimCompartment::getPixels() const {
  return comp.getPixels();
}

std::vector<double> &SimCompartment::getDcdt() { return dcdt; }

double SimCompartment::getMaxStableTimestep() const {
  return maxStableTimestep;
}

SimMembrane::SimMembrane(const sbml::SbmlDocWrapper &doc,
                         const geometry::Membrane &membrane_ptr,
                         SimCompartment &simCompA, SimCompartment &simCompB)
    : membrane(membrane_ptr), compA(simCompA), compB(simCompB) {
  if (membrane.compA->getId() != simCompA.getCompartmentId()) {
    SPDLOG_ERROR("compA '{}' doesn't match simCompA '{}'",
                 membrane.compA->getId(), simCompA.getCompartmentId());
  }
  if (membrane.compB->getId() != simCompB.getCompartmentId()) {
    SPDLOG_ERROR("compB '{}' doesn't match simCompB '{}'",
                 membrane.compB->getId(), simCompB.getCompartmentId());
  }
  SPDLOG_DEBUG("membrane: {}", membrane.membraneID);
  SPDLOG_DEBUG("  - compA: {}", compA.getCompartmentId());
  SPDLOG_DEBUG("  - compB: {}", compB.getCompartmentId());

  // make vector of species from compartments A and B
  auto speciesIds = compA.getSpeciesIds();
  speciesIds.insert(speciesIds.end(), compB.getSpeciesIds().cbegin(),
                    compB.getSpeciesIds().cend());

  // get rescaling factor to convert flux to amount/length^3,
  // then length^3 to volume to give concentration
  const auto &lengthUnit = doc.getModelUnits().getLength();
  const auto &volumeUnit = doc.getModelUnits().getVolume();
  double lengthCubedToVolFactor =
      units::pixelWidthToVolume(1.0, lengthUnit, volumeUnit);
  std::string strFactor =
      QString::number(lengthCubedToVolFactor, 'g', 17).toStdString();
  SPDLOG_INFO("  - [length]^3/[vol] = {}", lengthCubedToVolFactor);
  SPDLOG_INFO("  - dividing flux by {}", strFactor);

  // make vector of reaction IDs from membrane
  std::vector<std::string> reactionID =
      utils::toStdString(doc.reactions.at(membrane.membraneID.c_str()));
  // vector of reaction scale factors to convert flux to concentration
  std::vector<std::string> reactionScaleFactors(reactionID.size(), strFactor);

  reacEval = ReacEval(doc, speciesIds, reactionID, reactionScaleFactors);
}

void SimMembrane::evaluateReactions() {
  assert(reacEval.nSpecies ==
         compA.getSpeciesIds().size() + compB.getSpeciesIds().size());
  std::size_t nSpeciesA = compA.getSpeciesIds().size();
  std::size_t nSpeciesB = compB.getSpeciesIds().size();
  const auto &concA = compA.getConcentrations();
  const auto &concB = compB.getConcentrations();
  auto &dcdtA = compA.getDcdt();
  auto &dcdtB = compB.getDcdt();
  std::vector<double> species(nSpeciesA + nSpeciesB, 0);
  std::vector<double> result(nSpeciesA + nSpeciesB, 0);
  for (const auto &[ixA, ixB] : membrane.indexPair) {
    // populate species concentrations: first A, then B
    std::copy_n(&concA[ixA * nSpeciesA], nSpeciesA, &species[0]);
    std::copy_n(&concB[ixB * nSpeciesB], nSpeciesB, &species[nSpeciesA]);

    // evaluate reaction terms
    reacEval.evaluate(result.data(), species.data());

    // add results to dc/dt: first A, then B
    for (std::size_t is = 0; is < nSpeciesA; ++is) {
      dcdtA[ixA * nSpeciesA + is] += result[is];
    }
    for (std::size_t is = 0; is < nSpeciesB; ++is) {
      dcdtB[ixB * nSpeciesB + is] += result[is + nSpeciesA];
    }
  }
}

void PixelSim::calculateDcdt() {
  // calculate dcd/dt in all compartments
  for (auto &sim : simCompartments) {
    sim.evaluateReactions();
    sim.evaluateDiffusionOperator();
  }
  // membrane contribution to dc/dt
  for (auto &sim : simMembranes) {
    sim.evaluateReactions();
  }
  for (auto &sim : simCompartments) {
    sim.spatiallyAverageDcdt();
  }
}

void PixelSim::doRK101(double dt) {
  // RK1(0)1: Forwards Euler, no error estimate
  calculateDcdt();
  for (auto &sim : simCompartments) {
    sim.doForwardsEulerTimestep(dt);
  }
}

void PixelSim::doRK212(double dt) {
  // RK2(1)2: Heun / Modified Euler, with embedded forwards Euler error
  // estimate Shu-Osher form used here taken from eq(2.15) of
  // https://doi.org/10.1016/0021-9991(88)90177-5
  constexpr std::array<double, 2> g1{0.0, 0.0};
  constexpr std::array<double, 2> g2{0.0, 0.5};
  constexpr std::array<double, 2> g3{1.0, 0.5};
  constexpr std::array<double, 2> beta{1.0, 0.5};
  constexpr std::array<double, 2> delta{0.0, 1.0};
  for (std::size_t i = 0; i < 2; ++i) {
    calculateDcdt();
    for (auto &sim : simCompartments) {
      sim.doRKSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
    }
  }
}

void PixelSim::doRK323(double dt) {
  // RK3(2)3: Shu Osher method with embedded Heun error estimate
  // Taken from eq(2.18) of
  // https://doi.org/10.1016/0021-9991(88)90177-5
  constexpr std::array<double, 3> g1{1.0, 0.25, 2.0 / 3.0};
  constexpr std::array<double, 3> g2{0.0, 0.0, 0.0};
  constexpr std::array<double, 3> g3{0.0, 0.75, 1.0 / 3.0};
  constexpr std::array<double, 3> beta{1.0, 0.25, 2.0 / 3.0};
  constexpr std::array<double, 3> delta{0.0, 0.0, 1.0};
  for (std::size_t i = 0; i < 3; ++i) {
    calculateDcdt();
    for (auto &sim : simCompartments) {
      sim.doRKSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
    }
  }
  for (auto &sim : simCompartments) {
    sim.doRKFinalise(0.0, 2.0, -1.0);
  }
}

void PixelSim::doRK435(double dt) {
  // RK4(3)5: 3S* algorithm 6 + coefficients from table 6 from
  // https://doi.org/10.1016/j.jcp.2009.11.006
  // 5 stage RK4 with embedded RK3 error estimate
  constexpr std::array<double, 5> g1{0.0, -0.497531095840104, 1.010070514199942,
                                     -3.196559004608766, 1.717835630267259};
  constexpr std::array<double, 5> g2{1.0, 1.384996869124138, 3.878155713328178,
                                     -2.324512951813145, -0.514633322274467};
  constexpr std::array<double, 5> g3{0.0, 0.0, 0.0, 1.642598936063715,
                                     0.188295940828347};
  constexpr std::array<double, 5> beta{0.075152045700771, 0.211361016946069,
                                       1.100713347634329, 0.728537814675568,
                                       0.393172889823198};
  constexpr std::array<double, 7> delta{1.0,
                                        0.081252332929194,
                                        -1.083849060586449,
                                        -1.096110881845602,
                                        2.859440022030827,
                                        -0.655568367959557,
                                        -0.194421504490852};
  double deltaSum = 1.0 / utils::sum(delta);
  for (std::size_t i = 0; i < 5; ++i) {
    calculateDcdt();
    for (auto &sim : simCompartments) {
      sim.doRKSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
    }
  }
  for (auto &sim : simCompartments) {
    sim.doRKFinalise(deltaSum * delta[5], deltaSum, deltaSum * delta[6]);
  }
}

double PixelSim::doRKAdaptive(double dtMax) {
  // Adaptive timestep Runge-Kutta
  IntegratorError err;
  double dt;
  do {
    // do timestep
    dt = std::min(nextTimestep, dtMax);
    for (auto &sim : simCompartments) {
      sim.doRKInit();
    }
    if (integratorOrder == 2) {
      doRK212(dt);
    } else if (integratorOrder == 3) {
      doRK323(dt);
    } else if (integratorOrder == 4) {
      doRK435(dt);
    }
    // calculate error
    err.abs = 0;
    err.rel = 0;
    for (const auto &sim : simCompartments) {
      auto compErr = sim.calculateRKError(epsilon);
      err.rel = std::max(err.rel, compErr.rel);
      err.abs = std::max(err.abs, compErr.abs);
    }
    // calculate new timestep
    double errFactor = std::min(errMax.abs / err.abs, errMax.rel / err.rel);
    errFactor = std::pow(errFactor, 1.0 / static_cast<double>(integratorOrder));
    nextTimestep = std::min(0.95 * dt * errFactor, dtMax);
    SPDLOG_DEBUG("dt = {} gave rel err = {}, abs err = {} -> new dt = {}", dt,
                 err.rel, err.abs, nextTimestep);
    if (err.abs > errMax.abs || err.rel > errMax.rel) {
      SPDLOG_DEBUG("discarding step");
      ++discardedSteps;
      for (auto &sim : simCompartments) {
        sim.undoRKStep();
      }
    }
  } while (err.abs > errMax.abs || err.rel > errMax.rel);
  return dt;
}

PixelSim::PixelSim(const sbml::SbmlDocWrapper &sbmlDoc) : doc(sbmlDoc) {
  // add compartments
  for (const auto &compartmentID : doc.compartments) {
    simCompartments.emplace_back(doc,
                                 doc.mapCompIdToGeometry.at(compartmentID));
    maxStableTimestep = std::min(maxStableTimestep,
                                 simCompartments.back().getMaxStableTimestep());
  }
  // add membranes
  for (auto &membrane : doc.membraneVec) {
    if (doc.reactions.find(membrane.membraneID.c_str()) !=
        doc.reactions.cend()) {
      // find the two membrane compartments in simCompartments
      std::string compIdA = membrane.compA->getId();
      std::string compIdB = membrane.compB->getId();
      auto iterA = std::find_if(simCompartments.begin(), simCompartments.end(),
                                [&compIdA](const auto &c) {
                                  return c.getCompartmentId() == compIdA;
                                });
      if (iterA == simCompartments.cend()) {
        SPDLOG_ERROR("could not find compartment {} in simCompartments",
                     compIdA);
      }
      auto iterB = std::find_if(simCompartments.begin(), simCompartments.end(),
                                [&compIdB](const auto &c) {
                                  return c.getCompartmentId() == compIdB;
                                });
      if (iterB == simCompartments.cend()) {
        SPDLOG_ERROR("could not find compartment {} in simCompartments",
                     compIdB);
      }
      simMembranes.emplace_back(doc, membrane, *iterA, *iterB);
    }
  }
}

PixelSim::~PixelSim() = default;

void PixelSim::setIntegrationOrder(std::size_t order) {
  if (std::find(integratorOrders.cbegin(), integratorOrders.cend(), order) !=
      integratorOrders.cend()) {
    integratorOrder = order;
    SPDLOG_INFO("Setting integration order to {}", order);
  } else {
    SPDLOG_WARN("Integration order {} not supported", order);
  }
}

std::size_t PixelSim::getIntegrationOrder() const { return integratorOrder; }

void PixelSim::setIntegratorError(const IntegratorError &err) {
  SPDLOG_INFO("Setting max integration errors");
  SPDLOG_INFO("  - relative: {}", err.rel);
  SPDLOG_INFO("  - absolute: {}", err.abs);
  errMax = err;
}

IntegratorError PixelSim::getIntegratorError() const { return errMax; }

void PixelSim::setMaxDt(double maxDt) {
  SPDLOG_INFO("Setting max timestep: {}", maxDt);
  maxTimestep = maxDt;
}

double PixelSim::getMaxDt() const { return maxTimestep; }

std::size_t PixelSim::run(double time) {
  double tNow = 0;
  std::size_t steps = 0;
  discardedSteps = 0;
  // do timesteps until we reach t
  while (tNow + time * 1e-12 < time) {
    double maxDt = std::min(maxTimestep, time - tNow);
    if (integratorOrder > 1) {
      tNow += doRKAdaptive(maxDt);
    } else {
      double timestep = std::min(maxDt, maxStableTimestep);
      doRK101(timestep);
      tNow += timestep;
    }
    ++steps;
  }
  SPDLOG_INFO("t={} integrated using {} RK{} steps ({:3.1f}% discarded)", time,
              steps + discardedSteps, integratorOrder,
              static_cast<double>(100 * discardedSteps) /
                  static_cast<double>(steps + discardedSteps));
  return steps;
}

const std::vector<double> &PixelSim::getConcentrations(
    std::size_t compartmentIndex) const {
  return simCompartments[compartmentIndex].getConcentrations();
}

double PixelSim::getLowerOrderConcentration(std::size_t compartmentIndex,
                                            std::size_t speciesIndex,
                                            std::size_t pixelIndex) const {
  return simCompartments[compartmentIndex].getLowerOrderConcentration(
      speciesIndex, pixelIndex);
}

}  // namespace sim
