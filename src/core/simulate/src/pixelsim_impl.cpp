#include "pixelsim_impl.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "pde.hpp"
#include "utils.hpp"
#include <QString>
#include <QStringList>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_scheduler_init.h>
#endif

namespace simulate {

ReacEval::ReacEval(const model::Model &doc,
                   const std::vector<std::string> &speciesIDs,
                   const std::vector<std::string> &reactionIDs,
                   const std::vector<std::string> &reactionScaleFactors,
                   bool doCSE, unsigned optLevel) {
  // construct reaction expressions and stoich matrix
  PDE pde(&doc, speciesIDs, reactionIDs, {}, reactionScaleFactors);
  // compile all expressions with symengine
  sym = symbolic::Symbolic(pde.getRHS(), speciesIDs, {}, true, doCSE, optLevel);
}

void ReacEval::evaluate(double *output, const double *input) const {
  sym.eval(output, input);
}

void SimCompartment::spatiallyAverageDcdt() {
  // for any non-spatial species: spatially average dc/dt:
  // roughly equivalent to infinite rate of diffusion
  std::size_t nSpecies = speciesIds.size();
  std::size_t nPixels = comp->nPixels();
  for (std::size_t is : nonSpatialSpeciesIndices) {
    double av = 0;
    for (std::size_t ix = 0; ix < nPixels; ++ix) {
      av += dcdt[ix * nSpecies + is];
    }
    av /= static_cast<double>(nPixels);
    for (std::size_t ix = 0; ix < nPixels; ++ix) {
      dcdt[ix * nSpecies + is] = av;
    }
  }
}

SimCompartment::SimCompartment(const model::Model &doc,
                               const geometry::Compartment *compartment,
                               std::vector<std::string> sIds, bool doCSE,
                               unsigned optLevel)
    : comp{compartment}, compartmentId{compartment->getId()},
      speciesIds{std::move(sIds)} {
  // get species in compartment
  SPDLOG_DEBUG("compartment: {}", compartmentId);
  std::vector<const geometry::Field *> fields;
  for (const auto &s : speciesIds) {
    const auto *field = doc.getSpecies().getField(s.c_str());
    double pixelWidth = comp->getPixelWidth();
    diffConstants.push_back(field->getDiffusionConstant() / pixelWidth /
                            pixelWidth);
    // forwards euler stability bound: dt < a^2/4D
    maxStableTimestep =
        std::min(maxStableTimestep, 1.0 / (4.0 * diffConstants.back()));
    fields.push_back(field);
    if (!field->getIsSpatial()) {
      nonSpatialSpeciesIndices.push_back(fields.size() - 1);
    }
    SPDLOG_DEBUG("  - adding species: {}, diff constant {}", s,
                 diffConstants.back());
  }
  // get reactions in compartment
  std::vector<std::string> reactionIDs;
  if (auto reacsInCompartment =
          doc.getReactions().getIds(compartmentId.c_str());
      !reacsInCompartment.isEmpty()) {
    reactionIDs = utils::toStdString(reacsInCompartment);
  }
  reacEval = ReacEval(doc, speciesIds, reactionIDs, {}, doCSE, optLevel);
  // setup concentrations vector with initial values
  conc.resize(fields.size() * compartment->nPixels());
  dcdt.resize(conc.size(), 0.0);
  auto concIter = conc.begin();
  for (std::size_t ix = 0; ix < compartment->nPixels(); ++ix) {
    for (const auto *field : fields) {
      *concIter = field->getConcentration()[ix];
      ++concIter;
    }
  }
  assert(concIter == conc.end());
}

void SimCompartment::evaluateDiffusionOperator() {
  std::size_t ns = speciesIds.size();
  std::size_t np = comp->nPixels();
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(
      std::size_t{0}, np,
      [ns, &dcdt = dcdt, &diffConstants = std::as_const(diffConstants),
       &conc = std::as_const(conc),
       &comp = std::as_const(comp)](std::size_t i) {
#else
  for (std::size_t i = 0; i < np; ++i) {
#endif
        std::size_t ix = i * ns;
        std::size_t ix_upx = comp->up_x(i) * ns;
        std::size_t ix_dnx = comp->dn_x(i) * ns;
        std::size_t ix_upy = comp->up_y(i) * ns;
        std::size_t ix_dny = comp->dn_y(i) * ns;
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
  std::size_t N = ns * comp->nPixels();
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(
      std::size_t{0}, N, ns,
      [&reacEval = std::as_const(reacEval), &conc = std::as_const(conc),
       &dcdt = dcdt](std::size_t index) {
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
  std::size_t n{conc.size()};
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(
      std::size_t{0}, n,
      [dt, &conc = conc, &dcdt = std::as_const(dcdt)](std::size_t i) {
#else
  for (std::size_t i = 0; i < n; ++i) {
#endif
        conc[i] += dt * dcdt[i];
      }
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  );
#endif
}

void SimCompartment::doRKInit() {
  s2.assign(conc.size(), 0.0);
  s3 = conc;
}

void SimCompartment::doRKSubstep(double dt, double g1, double g2, double g3,
                                 double beta, double delta) {
  std::size_t n{conc.size()};
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(std::size_t{0}, n,
                    [&conc = conc, &s2 = s2, &s3 = std::as_const(s3),
                     &dcdt = std::as_const(dcdt), delta, g1, g2, g3, beta,
                     dt](std::size_t i) {
#else
  for (std::size_t i = 0; i < n; ++i) {
#endif
                      s2[i] += delta * conc[i];
                      conc[i] = g1 * conc[i] + g2 * s2[i] + g3 * s3[i] +
                                beta * dt * dcdt[i];
                    }
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  );
#endif
}

void SimCompartment::doRKFinalise(double cFactor, double s2Factor,
                                  double s3Factor) {
  std::size_t n{conc.size()};
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(
      std::size_t{0}, n,
      [&s2 = s2, &conc = std::as_const(conc), &s3 = std::as_const(s3), cFactor,
       s2Factor, s3Factor](std::size_t i) {
#else
  for (std::size_t i = 0; i < n; ++i) {
#endif
        s2[i] = cFactor * conc[i] + s2Factor * s2[i] + s3Factor * s3[i];
      }
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  );
#endif
}

void SimCompartment::undoRKStep() {
  std::size_t n{conc.size()};
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  tbb::parallel_for(std::size_t{0}, n,
                    [&conc = conc, &s3 = std::as_const(s3)](std::size_t i) {
#else
  for (std::size_t i = 0; i < n; ++i) {
#endif
                      conc[i] = s3[i];
                    }
#ifdef SPATIAL_MODEL_EDITOR_USE_TBB
  );
#endif
}

PixelIntegratorError SimCompartment::calculateRKError(double epsilon) const {
  PixelIntegratorError err{0.0, 0.0};
  std::size_t n{conc.size()};
  for (std::size_t i = 0; i < n; ++i) {
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

double
SimCompartment::getLowerOrderConcentration(std::size_t speciesIndex,
                                           std::size_t pixelIndex) const {
  if (s2.empty()) {
    return 0;
  }
  std::size_t nSpecies = speciesIds.size();
  return s2[pixelIndex * nSpecies + speciesIndex];
}

const std::vector<QPoint> &SimCompartment::getPixels() const {
  return comp->getPixels();
}

std::vector<double> &SimCompartment::getDcdt() { return dcdt; }

double SimCompartment::getMaxStableTimestep() const {
  return maxStableTimestep;
}

SimMembrane::SimMembrane(const model::Model &doc,
                         const geometry::Membrane *membrane_ptr,
                         SimCompartment *simCompA, SimCompartment *simCompB,
                         bool doCSE, unsigned optLevel)
    : membrane(membrane_ptr), compA(simCompA), compB(simCompB) {
  if (compA != nullptr &&
      membrane->getCompartmentA()->getId() != compA->getCompartmentId()) {
    SPDLOG_ERROR("compA '{}' doesn't match simCompA '{}'",
                 membrane->getCompartmentA()->getId(),
                 compA->getCompartmentId());
  }
  if (compB != nullptr &&
      membrane->getCompartmentB()->getId() != compB->getCompartmentId()) {
    SPDLOG_ERROR("compB '{}' doesn't match simCompB '{}'",
                 membrane->getCompartmentB()->getId(),
                 compB->getCompartmentId());
  }
  SPDLOG_DEBUG("membrane: {}", membrane->getId());
  SPDLOG_DEBUG("  - compA: {}",
               compA != nullptr ? compA->getCompartmentId() : "");
  SPDLOG_DEBUG("  - compB: {}",
               compB != nullptr ? compB->getCompartmentId() : "");

  // make vector of species from compartments A and B
  std::vector<std::string> speciesIds;
  for (const auto *c : {compA, compB}) {
    if (c != nullptr) {
      speciesIds.insert(speciesIds.end(), c->getSpeciesIds().cbegin(),
                        c->getSpeciesIds().cend());
    }
  }

  // get rescaling factor to convert flux to amount/length^3,
  // then length^3 to volume to give concentration
  const auto &lengthUnit = doc.getUnits().getLength();
  const auto &volumeUnit = doc.getUnits().getVolume();
  double lengthCubedToVolFactor =
      model::pixelWidthToVolume(1.0, lengthUnit, volumeUnit);
  std::string strFactor =
      QString::number(lengthCubedToVolFactor, 'g', 17).toStdString();
  SPDLOG_INFO("  - [length]^3/[vol] = {}", lengthCubedToVolFactor);
  SPDLOG_INFO("  - dividing flux by {}", strFactor);

  // make vector of reaction IDs from membrane
  std::vector<std::string> reactionID =
      utils::toStdString(doc.getReactions().getIds(membrane->getId().c_str()));
  // vector of reaction scale factors to convert flux to concentration
  std::vector<std::string> reactionScaleFactors(reactionID.size(), strFactor);

  reacEval = ReacEval(doc, speciesIds, reactionID, reactionScaleFactors, doCSE,
                      optLevel);
}

void SimMembrane::evaluateReactions() {
  std::size_t nSpeciesA{0};
  const std::vector<double> *concA{nullptr};
  std::vector<double> *dcdtA{nullptr};
  if (compA != nullptr) {
    nSpeciesA = compA->getSpeciesIds().size();
    concA = &compA->getConcentrations();
    dcdtA = &compA->getDcdt();
  }
  std::size_t nSpeciesB{0};
  const std::vector<double> *concB{nullptr};
  std::vector<double> *dcdtB{nullptr};
  if (compB != nullptr) {
    nSpeciesB = compB->getSpeciesIds().size();
    concB = &compB->getConcentrations();
    dcdtB = &compB->getDcdt();
  }
  std::vector<double> species(nSpeciesA + nSpeciesB, 0);
  std::vector<double> result(nSpeciesA + nSpeciesB, 0);
  for (const auto &[ixA, ixB] : membrane->getIndexPairs()) {
    // populate species concentrations: first A, then B
    if (concA != nullptr) {
      std::copy_n(&((*concA)[ixA * nSpeciesA]), nSpeciesA, &species[0]);
    }
    if (concB != nullptr) {
      std::copy_n(&((*concB)[ixB * nSpeciesB]), nSpeciesB, &species[nSpeciesA]);
    }

    // evaluate reaction terms
    reacEval.evaluate(result.data(), species.data());

    // add results to dc/dt: first A, then B
    for (std::size_t is = 0; is < nSpeciesA; ++is) {
      (*dcdtA)[ixA * nSpeciesA + is] += result[is];
    }
    for (std::size_t is = 0; is < nSpeciesB; ++is) {
      (*dcdtB)[ixB * nSpeciesB + is] += result[is + nSpeciesA];
    }
  }
}

} // namespace simulate
