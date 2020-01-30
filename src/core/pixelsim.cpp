#include "pixelsim.hpp"

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
    : result(speciesIDs.size(), 0.0),
      species_values(speciesIDs.size(), 0.0),
      nSpecies(speciesIDs.size()) {
  // construct reaction expressions and stoich matrix
  pde::PDE pde(&doc, speciesIDs, reactionIDs, {}, reactionScaleFactors);
  // compile all expressions with symengine
  sym = symbolic::Symbolic(pde.getRHS(), speciesIDs);
}

void ReacEval::evaluate() { sym.eval(result, species_values); }

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
  std::size_t nSpecies = speciesIds.size();
  for (std::size_t ix = 0; ix < comp.nPixels(); ++ix) {
    std::size_t ix_upx = comp.up_x(ix);
    std::size_t ix_dnx = comp.dn_x(ix);
    std::size_t ix_upy = comp.up_y(ix);
    std::size_t ix_dny = comp.dn_y(ix);
    for (std::size_t is = 0; is < nSpecies; ++is) {
      dcdt[ix * nSpecies + is] =
          diffConstants[is] *
          (conc[ix_upx * nSpecies + is] + conc[ix_dnx * nSpecies + is] +
           conc[ix_upy * nSpecies + is] + conc[ix_dny * nSpecies + is] -
           4.0 * conc[ix * nSpecies + is]);
    }
  }
}

void SimCompartment::evaluateReactions() {
  std::size_t nSpecies = speciesIds.size();
  for (std::size_t ix = 0; ix < comp.nPixels(); ++ix) {
    // populate species concentrations
    std::copy_n(&conc[ix * nSpecies], nSpecies,
                reacEval.species_values.begin());
    reacEval.evaluate();
    const std::vector<double> &result = reacEval.getResult();
    // add results to dcdt
    for (std::size_t is = 0; is < nSpecies; ++is) {
      dcdt[ix * nSpecies + is] += result[is];
    }
  }
}

void SimCompartment::doForwardsEulerTimestep(double dt) {
  // first spatially average dc/dt for any non-spatial species
  spatiallyAverageDcdt();
  // do forwards euler timestep
  for (std::size_t i = 0; i < conc.size(); ++i) {
    conc[i] += dt * dcdt[i];
  }
}

const std::string &SimCompartment::getCompartmentId() const {
  return compartmentId;
};

const std::vector<std::string> &SimCompartment::getSpeciesIds() const {
  return speciesIds;
}

const std::vector<double> &SimCompartment::getConcentrations() const {
  return conc;
}

const std::vector<QPoint> &SimCompartment::getPixels() const {
  return comp.getPixels();
}

std::vector<double> &SimCompartment::getDcdt() { return dcdt; }

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
  assert(reacEval.species_values.size() ==
         compA.getSpeciesIds().size() + compB.getSpeciesIds().size());
  std::size_t nSpeciesA = compA.getSpeciesIds().size();
  std::size_t nSpeciesB = compB.getSpeciesIds().size();
  const auto &concA = compA.getConcentrations();
  const auto &concB = compB.getConcentrations();
  auto &dcdtA = compA.getDcdt();
  auto &dcdtB = compB.getDcdt();
  for (const auto &[ixA, ixB] : membrane.indexPair) {
    // populate species concentrations: first A, then B
    auto iterReacValues = reacEval.species_values.begin();
    std::copy_n(&concA[ixA * nSpeciesA], nSpeciesA, iterReacValues);
    std::advance(iterReacValues, nSpeciesA);
    std::copy_n(&concB[ixB * nSpeciesB], nSpeciesB, iterReacValues);

    // evaluate reaction terms
    reacEval.evaluate();
    const std::vector<double> &result = reacEval.getResult();

    // add results to dc/dt: first A, then B
    for (std::size_t is = 0; is < nSpeciesA; ++is) {
      dcdtA[ixA * nSpeciesA + is] += result[is];
    }
    for (std::size_t is = 0; is < nSpeciesB; ++is) {
      dcdtB[ixB * nSpeciesB + is] += result[is + nSpeciesA];
    }
  }
}

PixelSim::PixelSim(const sbml::SbmlDocWrapper &sbmlDoc) : doc(sbmlDoc) {
  // add compartments
  for (const auto &compartmentID : doc.compartments) {
    simCompartments.emplace_back(doc,
                                 doc.mapCompIdToGeometry.at(compartmentID));
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

void PixelSim::doTimestep(double t, double dt) {
  double tNow = 0;
  while (tNow < t) {
    // calculate dcd/dt in all compartments
    for (auto &sim : simCompartments) {
      sim.evaluateDiffusionOperator();
      sim.evaluateReactions();
    }
    // membrane contribution to dc/dt
    for (auto &sim : simMembranes) {
      sim.evaluateReactions();
    }
    for (auto &sim : simCompartments) {
      sim.doForwardsEulerTimestep(dt);
    }
    tNow += dt;
  }
}

const std::vector<double> &PixelSim::getConcentrations(
    std::size_t compartmentIndex) const {
  return simCompartments[compartmentIndex].getConcentrations();
}

}  // namespace sim
