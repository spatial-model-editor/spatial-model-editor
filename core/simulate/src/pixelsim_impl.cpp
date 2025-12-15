#include "pixelsim_impl.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/pde.hpp"
#include "sme/utils.hpp"
#include <QString>
#include <QStringList>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>
// Qt defines emit keyword which interferes with a tbb emit() function
#ifdef emit
#undef emit
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/tick_count.h>
#define emit // restore the Qt empty definition of "emit"
#else
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/tick_count.h>
#endif

namespace sme::simulate {

class PixelSimImplError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

template <typename Body>
static void tbbParallelFor(std::size_t n, const Body &body) {
  constexpr std::size_t tbbGrainSize{64};
  static oneapi::tbb::static_partitioner partitioner;
  oneapi::tbb::parallel_for(
      oneapi::tbb::blocked_range<std::size_t>(0, n, tbbGrainSize), body,
      partitioner);
}

ReacExpr::ReacExpr(
    const model::Model &doc, const std::vector<std::string> &speciesIDs,
    const std::vector<std::string> &reactionIDs, double reactionScaleFactor,
    bool timeDependent, bool spaceDependent,
    const std::map<std::string, double, std::less<>> &substitutions) {
  // construct reaction expressions and variables
  PdeScaleFactors pdeScaleFactors;
  pdeScaleFactors.reaction = reactionScaleFactor;
  std::vector<std::string> extraVars{};
  if (timeDependent) {
    SPDLOG_TRACE("model reactions depend on time");
    extraVars.push_back("time");
  }
  if (spaceDependent) {
    SPDLOG_TRACE("model reactions depend on space");
    extraVars.push_back(doc.getParameters().getSpatialCoordinates().x.id);
    extraVars.push_back(doc.getParameters().getSpatialCoordinates().y.id);
  }
  Pde pde(&doc, speciesIDs, reactionIDs, {}, pdeScaleFactors, extraVars, {},
          substitutions);
  // add dt/dt = 1 reaction term, and t,x,y "species"
  variables = speciesIDs;
  variables.insert(variables.end(), extraVars.cbegin(), extraVars.cend());
  expressions = pde.getRHS();
  if (timeDependent) {
    expressions.emplace_back("1"); // dt/dt = 1
  }
  if (spaceDependent) {
    expressions.emplace_back("0"); // dx/dt = 0
    expressions.emplace_back("0"); // dy/dt = 0
  }
}

void SimCompartment::spatiallyAverageDcdt() {
  // for any non-spatial species: spatially average dc/dt:
  // roughly equivalent to infinite rate of diffusion
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

// Forwards Euler stability bound for dimensionless diffusion constant {D/dx^2,
// D/dy^2, D/dz^2}
static double calculateMaxStableTimestep(
    const std::array<double, 3> &dimensionlessDiffusion) {
  return 1.0 / (2.0 * (dimensionlessDiffusion[0] + dimensionlessDiffusion[1] +
                       dimensionlessDiffusion[2]));
}

SimCompartment::SimCompartment(
    const model::Model &doc, const geometry::Compartment *compartment,
    std::vector<std::string> sIds, bool doCSE, unsigned optLevel,
    bool timeDependent, bool spaceDependent,
    const std::map<std::string, double, std::less<>> &substitutions)
    : comp{compartment}, nPixels{compartment->nVoxels()}, nSpecies{sIds.size()},
      compartmentId{compartment->getId()}, speciesIds{std::move(sIds)} {
  // get species in compartment
  speciesNames.reserve(nSpecies);
  SPDLOG_DEBUG("compartment: {}", compartmentId);
  std::vector<const geometry::Field *> fields;
  const auto voxelSize{doc.getGeometry().getVoxelSize()};
  dx2 = voxelSize.width() * voxelSize.width();
  dy2 = voxelSize.height() * voxelSize.height();
  dz2 = voxelSize.depth() * voxelSize.depth();
  for (const auto &s : speciesIds) {
    const auto *field = doc.getSpecies().getField(s.c_str());
    // store diffusion constant per voxel
    diffConstants.push_back(field->getDiffusionConstant());
    if (diffConstants.back().empty()) {
      diffConstants.back().assign(nPixels, 0.0);
    }
    // forwards euler stability bound (use max D for species)
    double maxD{diffConstants.back().empty()
                    ? 0.0
                    : *std::ranges::max_element(diffConstants.back())};
    maxStableTimestep = std::min(
        maxStableTimestep,
        calculateMaxStableTimestep({maxD / dx2, maxD / dy2, maxD / dz2}));
    fields.push_back(field);
    speciesNames.push_back(doc.getSpecies().getName(s.c_str()).toStdString());
    if (!field->getIsSpatial()) {
      nonSpatialSpeciesIndices.push_back(fields.size() - 1);
    }
    SPDLOG_DEBUG("  - adding species: {}, diff constant (max) {}", s, maxD);
  }
  // get reactions in compartment
  std::vector<std::string> reactionIDs;
  if (auto reacsInCompartment =
          doc.getReactions().getIds(compartmentId.c_str());
      !reacsInCompartment.isEmpty()) {
    reactionIDs = common::toStdString(reacsInCompartment);
  }
  ReacExpr reacExpr(doc, speciesIds, reactionIDs, 1.0, timeDependent,
                    spaceDependent, substitutions);
  if (!(sym.parse(reacExpr.expressions, reacExpr.variables) &&
        sym.compile(doCSE, optLevel))) {
    throw PixelSimImplError(sym.getErrorMessage());
  }
  if (timeDependent) {
    speciesIds.push_back("time");
    diffConstants.emplace_back(nPixels, 0.0);
    ++nSpecies;
  }
  if (spaceDependent) {
    speciesIds.push_back(doc.getParameters().getSpatialCoordinates().x.id);
    diffConstants.emplace_back(nPixels, 0.0);
    speciesIds.push_back(doc.getParameters().getSpatialCoordinates().y.id);
    diffConstants.emplace_back(nPixels, 0.0);
    // todo: make sure we can still import previous sim results without z
    // OR note in release that existing simulation results with spatially
    // dependent reaction terms will not be imported - as most likely this
    // affects no-one.
    speciesIds.push_back(doc.getParameters().getSpatialCoordinates().z.id);
    diffConstants.emplace_back(nPixels, 0.0);
    nSpecies += 3;
  }
  // setup concentrations vector with initial values
  conc.resize(nSpecies * nPixels);
  dcdt.resize(conc.size(), 0.0);
  auto origin{doc.getGeometry().getPhysicalOrigin()};
  auto concIter{conc.begin()};
  for (std::size_t ix = 0; ix < compartment->nVoxels(); ++ix) {
    for (const auto *field : fields) {
      *concIter = field->getConcentration()[ix];
      ++concIter;
    }
    if (timeDependent) {
      *concIter = 0; // t
      ++concIter;
    }
    if (spaceDependent) {
      auto voxel{compartment->getVoxel(ix)};
      int ny{compartment->getCompartmentImages()[0].height()};
      *concIter = origin.p.x() +
                  static_cast<double>(voxel.p.x()) * voxelSize.width(); // x
      ++concIter;
      // pixels have y=0 in top-left, convert to bottom-left:
      *concIter = origin.p.y() + static_cast<double>(ny - 1 - voxel.p.y()) *
                                     voxelSize.height(); // y
      ++concIter;
      *concIter =
          origin.z + static_cast<double>(voxel.z) * voxelSize.depth(); // z
      ++concIter;
    }
  }
  assert(concIter == conc.end());
}

void SimCompartment::evaluateDiffusionOperator(std::size_t begin,
                                               std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    const std::size_t ix{i * nSpecies};
    const std::size_t ix_upx{comp->up_x(i) * nSpecies};
    const std::size_t ix_dnx{comp->dn_x(i) * nSpecies};
    const std::size_t ix_upy{comp->up_y(i) * nSpecies};
    const std::size_t ix_dny{comp->dn_y(i) * nSpecies};
    const std::size_t ix_upz{comp->up_z(i) * nSpecies};
    const std::size_t ix_dnz{comp->dn_z(i) * nSpecies};
    for (std::size_t is = 0; is < nSpecies; ++is) {
      const auto &d = diffConstants[is];
      double d_i = d[i];
      double d_upx = d[comp->up_x(i)];
      double d_dnx = d[comp->dn_x(i)];
      double d_upy = d[comp->up_y(i)];
      double d_dny = d[comp->dn_y(i)];
      double d_upz = d[comp->up_z(i)];
      double d_dnz = d[comp->dn_z(i)];
      double dxp = 0.5 * (d_i + d_upx) / dx2;
      double dxm = 0.5 * (d_i + d_dnx) / dx2;
      double dyp = 0.5 * (d_i + d_upy) / dy2;
      double dym = 0.5 * (d_i + d_dny) / dy2;
      double dzp = 0.5 * (d_i + d_upz) / dz2;
      double dzm = 0.5 * (d_i + d_dnz) / dz2;
      dcdt[ix + is] += dxp * (conc[ix_upx + is] - conc[ix + is]) -
                       dxm * (conc[ix + is] - conc[ix_dnx + is]) +
                       dyp * (conc[ix_upy + is] - conc[ix + is]) -
                       dym * (conc[ix + is] - conc[ix_dny + is]) +
                       dzp * (conc[ix_upz + is] - conc[ix + is]) -
                       dzm * (conc[ix + is] - conc[ix_dnz + is]);
    }
  }
}

void SimCompartment::evaluateReactions(std::size_t begin, std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    sym.eval(dcdt.data() + i * nSpecies, conc.data() + i * nSpecies);
  }
}

void SimCompartment::evaluateReactionsAndDiffusion() {
  evaluateReactions(0, nPixels);
  evaluateDiffusionOperator(0, nPixels);
}

void SimCompartment::evaluateReactionsAndDiffusion_tbb() {
  tbbParallelFor(nPixels,
                 [this](const oneapi::tbb::blocked_range<std::size_t> &r) {
                   evaluateReactions(r.begin(), r.end());
                   evaluateDiffusionOperator(r.begin(), r.end());
                 });
}

void SimCompartment::doForwardsEulerTimestep(double dt, std::size_t begin,
                                             std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    conc[i] += dt * dcdt[i];
  }
}

void SimCompartment::doForwardsEulerTimestep(double dt) {
  doForwardsEulerTimestep(dt, 0, conc.size());
}

void SimCompartment::doForwardsEulerTimestep_tbb(double dt) {
  tbbParallelFor(conc.size(),
                 [this, dt](const oneapi::tbb::blocked_range<std::size_t> &r) {
                   doForwardsEulerTimestep(dt, r.begin(), r.end());
                 });
}

void SimCompartment::doRKInit() {
  s2.assign(conc.size(), 0.0);
  s3 = conc;
}

void SimCompartment::doRK212Substep1(double dt, std::size_t begin,
                                     std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    s3[i] = conc[i];
    conc[i] += dt * dcdt[i];
  }
}

void SimCompartment::doRK212Substep1(double dt) {
  s2.resize(conc.size());
  s3.resize(conc.size());
  doRK212Substep1(dt, 0, conc.size());
}

void SimCompartment::doRK212Substep1_tbb(double dt) {
  s2.resize(conc.size());
  s3.resize(conc.size());
  tbbParallelFor(conc.size(),
                 [this, dt](const oneapi::tbb::blocked_range<std::size_t> &r) {
                   doRK212Substep1(dt, r.begin(), r.end());
                 });
}

void SimCompartment::doRK212Substep2(double dt, std::size_t begin,
                                     std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    s2[i] = conc[i];
    conc[i] = 0.5 * s3[i] + 0.5 * conc[i] + 0.5 * dt * dcdt[i];
  }
}

void SimCompartment::doRK212Substep2(double dt) {
  doRK212Substep2(dt, 0, conc.size());
}

void SimCompartment::doRK212Substep2_tbb(double dt) {
  tbbParallelFor(conc.size(),
                 [this, dt](const oneapi::tbb::blocked_range<std::size_t> &r) {
                   doRK212Substep2(dt, r.begin(), r.end());
                 });
}

void SimCompartment::doRKSubstep(double dt, double g1, double g2, double g3,
                                 double beta, double delta, std::size_t begin,
                                 std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    s2[i] += delta * conc[i];
    conc[i] = g1 * conc[i] + g2 * s2[i] + g3 * s3[i] + beta * dt * dcdt[i];
  }
}

void SimCompartment::doRKSubstep(double dt, double g1, double g2, double g3,
                                 double beta, double delta) {
  doRKSubstep(dt, g1, g2, g3, beta, delta, 0, conc.size());
}

void SimCompartment::doRKSubstep_tbb(double dt, double g1, double g2, double g3,
                                     double beta, double delta) {
  tbbParallelFor(conc.size(),
                 [this, dt, g1, g2, g3, beta,
                  delta](const oneapi::tbb::blocked_range<std::size_t> &r) {
                   doRKSubstep(dt, g1, g2, g3, beta, delta, r.begin(), r.end());
                 });
}

void SimCompartment::doRKFinalise(double cFactor, double s2Factor,
                                  double s3Factor, std::size_t begin,
                                  std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    s2[i] = cFactor * conc[i] + s2Factor * s2[i] + s3Factor * s3[i];
  }
}

void SimCompartment::doRKFinalise(double cFactor, double s2Factor,
                                  double s3Factor) {
  doRKFinalise(cFactor, s2Factor, s3Factor, 0, conc.size());
}

void SimCompartment::doRKFinalise_tbb(double cFactor, double s2Factor,
                                      double s3Factor) {
  tbbParallelFor(
      conc.size(), [this, cFactor, s2Factor, s3Factor](
                       const oneapi::tbb::blocked_range<std::size_t> &r) {
        doRKFinalise(cFactor, s2Factor, s3Factor, r.begin(), r.end());
      });
}

void SimCompartment::undoRKStep(std::size_t begin, std::size_t end) {
  for (std::size_t i = begin; i < end; ++i) {
    conc[i] = s3[i];
  }
}

void SimCompartment::undoRKStep() { undoRKStep(0, conc.size()); }

void SimCompartment::undoRKStep_tbb() {
  tbbParallelFor(conc.size(),
                 [this](const oneapi::tbb::blocked_range<std::size_t> &r) {
                   undoRKStep(r.begin(), r.end());
                 });
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

std::string SimCompartment::plotRKError(common::ImageStack &images,
                                        double epsilon, double max) const {
  auto imageSize{comp->getImageSize()};
  images = {imageSize, QImage::Format_RGB32};
  images.fill(0);
  std::size_t iSpecies{nSpecies + 1};
  for (std::size_t i = 0; i < conc.size(); ++i) {
    double localErr = std::abs(conc[i] - s2[i]);
    double localNorm = 0.5 * (conc[i] + s3[i] + epsilon);
    double pixelIntensity{localErr / localNorm / max};
    auto red{static_cast<int>(255.0 * pixelIntensity)};
    auto voxel{comp->getVoxel(i / nSpecies)};
    auto oldRed{qRed(images[voxel.z].pixel(voxel.p))};
    if (red > oldRed) {
      images[voxel.z].setPixel(voxel.p, qRgb(red, 0, 0));
      if (red > 254) {
        // update index of species with largest error
        iSpecies = i % nSpecies;
      }
    }
  }
  if (iSpecies < nSpecies) {
    return speciesNames[iSpecies];
  }
  return {};
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

void SimCompartment::setConcentrations(
    const std::vector<double> &concentrations) {
  conc = concentrations;
}

double
SimCompartment::getLowerOrderConcentration(std::size_t speciesIndex,
                                           std::size_t pixelIndex) const {
  if (s2.empty()) {
    return 0;
  }
  return s2[pixelIndex * nSpecies + speciesIndex];
}

const std::vector<common::Voxel> &SimCompartment::getVoxels() const {
  return comp->getVoxels();
}

std::vector<double> &SimCompartment::getDcdt() { return dcdt; }

double SimCompartment::getMaxStableTimestep() const {
  return maxStableTimestep;
}

SimMembrane::SimMembrane(
    const model::Model &doc, const geometry::Membrane *membrane_ptr,
    SimCompartment *simCompA, SimCompartment *simCompB, bool doCSE,
    unsigned optLevel, bool timeDependent, bool spaceDependent,
    const std::map<std::string, double, std::less<>> &substitutions)
    : membrane(membrane_ptr), compA(simCompA), compB(simCompB),
      voxelSize{doc.getGeometry().getVoxelSize()} {
  if (timeDependent) {
    ++nExtraVars;
  }
  if (spaceDependent) {
    nExtraVars += 3;
  }
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
  if (compA != nullptr) {
    for (std::size_t i = 0; i < compA->getSpeciesIds().size() - nExtraVars;
         ++i) {
      speciesIds.push_back(compA->getSpeciesIds()[i]);
    }
  }
  if (compB != nullptr) {
    for (std::size_t i = 0; i < compB->getSpeciesIds().size() - nExtraVars;
         ++i) {
      speciesIds.push_back(compB->getSpeciesIds()[i]);
    }
  }

  /* We want to convert the user-provided flux
   *
   * R = delta amount of species / membrane area
   *
   * to a change in the concentration of species in the voxel.
   *
   * Assuming the membrane surface is in the y-z plane,
   * with the flux going in the x direction:
   *
   * R * dy * dz = delta amount
   * R * dy * dz / (dx * dy * dz) = R / dx = delta amount / volume[length units]
   * R * volOverL3 / dx = delta concentration
   *
   * Since dx here is length of the voxel in the flux direction,
   * it depends on the orientation of each membrane face, and we will deal
   * with it separately for each voxel pair during the simulation.
   * Here we only include the volOverL3 factor, so that our reacEval provides
   *
   * delta concentration * voxel length in flux direction
   *
   */
  const auto &lengthUnit = doc.getUnits().getLength();
  const auto &volumeUnit = doc.getUnits().getVolume();
  double volOverL3 = model::getVolOverL3(lengthUnit, volumeUnit);
  SPDLOG_INFO("  - [vol]/[length]^3 = {}", volOverL3);
  SPDLOG_INFO("  - multiplying reaction by '{}'", volOverL3);
  // make vector of reaction IDs from membrane
  std::vector<std::string> reactionID =
      common::toStdString(doc.getReactions().getIds(membrane->getId().c_str()));
  ReacExpr reacExpr(doc, speciesIds, reactionID, volOverL3, timeDependent,
                    spaceDependent, substitutions);
  if (!(sym.parse(reacExpr.expressions, reacExpr.variables) &&
        sym.compile(doCSE, optLevel))) {
    throw PixelSimImplError(sym.getErrorMessage());
  }
}

void SimMembrane::evaluateReactions() {
  std::size_t nSpeciesA{0};
  const std::vector<double> *concA{nullptr};
  std::vector<double> *dcdtA{nullptr};
  if (compA != nullptr) {
    nSpeciesA = compA->getSpeciesIds().size() - nExtraVars;
    concA = &compA->getConcentrations();
    dcdtA = &compA->getDcdt();
  }
  std::size_t nSpeciesB{0};
  const std::vector<double> *concB{nullptr};
  std::vector<double> *dcdtB{nullptr};
  if (compB != nullptr) {
    nSpeciesB = compB->getSpeciesIds().size() - nExtraVars;
    concB = &compB->getConcentrations();
    dcdtB = &compB->getDcdt();
  }
  std::vector<double> species(nSpeciesA + nSpeciesB + nExtraVars, 0);
  std::vector<double> result(nSpeciesA + nSpeciesB + nExtraVars, 0);
  for (const auto &[fluxDir, fluxLength] :
       std::array<std::pair<geometry::Membrane::FLUX_DIRECTION, double>, 3>{
           {{geometry::Membrane::FLUX_DIRECTION::X, voxelSize.width()},
            {geometry::Membrane::FLUX_DIRECTION::Y, voxelSize.height()},
            {geometry::Membrane::FLUX_DIRECTION::Z, voxelSize.depth()}}}) {
    for (const auto &[ixA, ixB] : membrane->getIndexPairs(fluxDir)) {
      // populate species concentrations: first A, then B, then t,x,y,z
      if (concA != nullptr) {
        std::copy_n(&((*concA)[ixA * (nSpeciesA + nExtraVars)]), nSpeciesA,
                    &species[0]);
      }
      if (concB != nullptr) {
        std::copy_n(&((*concB)[ixB * (nSpeciesB + nExtraVars)]),
                    nSpeciesB + nExtraVars, &species[nSpeciesA]);
      } else if (concA != nullptr) {
        std::copy_n(&((*concA)[ixA * (nSpeciesA + nExtraVars) + nSpeciesA]),
                    nExtraVars, &species[nSpeciesA]);
      }

      // evaluate reaction terms
      sym.eval(result.data(), species.data());

      // add results to dc/dt: first A, then B. divide by fluxLength to get
      // change in concentration for this voxel
      for (std::size_t is = 0; is < nSpeciesA; ++is) {
        (*dcdtA)[ixA * (nSpeciesA + nExtraVars) + is] +=
            result[is] / fluxLength;
      }
      for (std::size_t is = 0; is < nSpeciesB; ++is) {
        (*dcdtB)[ixB * (nSpeciesB + nExtraVars) + is] +=
            result[is + nSpeciesA] / fluxLength;
      }
    }
  }
}

} // namespace sme::simulate
