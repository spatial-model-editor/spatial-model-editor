#include "simulate.hpp"

#include "geometry.hpp"
#include "logger.hpp"
#include "pde.hpp"
#include "sbml.hpp"
#include "utils.hpp"

namespace simulate {

ReacEval::ReacEval(sbml::SbmlDocWrapper *doc_ptr,
                   const std::vector<std::string> &speciesIDs,
                   const std::vector<std::string> &reactionIDs,
                   const std::vector<std::string> &reactionScaleFactors) {
  // construct reaction expressions and stoich matrix
  pde::PDE pde(doc_ptr, speciesIDs, reactionIDs, {}, reactionScaleFactors);

  // init vector of species
  species_values = std::vector<double>(speciesIDs.size(), 0.0);
  result = species_values;
  nSpecies = species_values.size();

  // compile all expressions with symengine
  reac_eval_symengine = symbolic::Symbolic(pde.getRHS(), speciesIDs);
}

void ReacEval::evaluate() { reac_eval_symengine.eval(result, species_values); }

SimCompartment::SimCompartment(sbml::SbmlDocWrapper *docWrapper,
                               const geometry::Compartment *compartment)
    : doc(docWrapper), comp(compartment) {
  QString compID = compartment->compartmentID.c_str();
  SPDLOG_DEBUG("compartment: {}", compID.toStdString());
  std::vector<std::string> speciesID;
  for (const auto &s : doc->species.at(compID)) {
    if (!doc->getIsSpeciesConstant(s.toStdString())) {
      speciesID.push_back(s.toStdString());
      field.push_back(&doc->mapSpeciesIdToField.at(s));
      SPDLOG_DEBUG("  - adding field: {}", s.toStdString());
    }
  }
  std::vector<std::string> reactionID;
  const auto iter = doc->reactions.find(compID);
  if (iter != doc->reactions.cend()) {
    reactionID = utils::toStdString(iter->second);
  }
  reacEval = ReacEval(doc, speciesID, reactionID, {});
}

void SimCompartment::evaluate_reactions() {
  for (std::size_t i = 0; i < comp->ix.size(); ++i) {
    // populate species concentrations
    for (std::size_t s = 0; s < field.size(); ++s) {
      reacEval.species_values[s] = field[s]->conc[i];
    }
    reacEval.evaluate();
    const std::vector<double> &result = reacEval.getResult();
    for (std::size_t s = 0; s < field.size(); ++s) {
      // add results to dcdt
      field[s]->dcdt[i] += result[s];
    }
  }
}

SimMembrane::SimMembrane(sbml::SbmlDocWrapper *doc_ptr,
                         geometry::Membrane *membrane_ptr)
    : doc(doc_ptr), membrane(membrane_ptr) {
  QString compA = membrane->compA->compartmentID.c_str();
  QString compB = membrane->compB->compartmentID.c_str();
  SPDLOG_DEBUG("membrane: {}", membrane->membraneID);
  SPDLOG_DEBUG("  - compA: {}", compA.toStdString());
  SPDLOG_DEBUG("  - compB: {}", compB.toStdString());

  // make vector of species & fields from compartments A and B
  std::vector<std::string> speciesID;
  for (const auto &spec : doc->species.at(compA)) {
    if (!doc->getIsSpeciesConstant(spec.toStdString())) {
      speciesID.push_back(spec.toStdString());
      fieldA.push_back(&doc->mapSpeciesIdToField.at(spec));
    }
  }
  for (const auto &spec : doc->species.at(compB)) {
    if (!doc->getIsSpeciesConstant(spec.toStdString())) {
      speciesID.push_back(spec.toStdString());
      fieldB.push_back(&doc->mapSpeciesIdToField.at(spec));
    }
  }

  // get rescaling factor to convert flux to amount/length^3,
  // then length^3 to volume to give concentration
  const auto &lengthUnit = doc->getModelUnits().getLength();
  const auto &volumeUnit = doc->getModelUnits().getVolume();
  double lengthCubedToVolFactor =
      units::pixelWidthToVolume(1.0, lengthUnit, volumeUnit);
  std::string strFactor =
      QString::number(lengthCubedToVolFactor, 'g', 17).toStdString();
  SPDLOG_INFO("  - [length]^3/[vol] = {}", lengthCubedToVolFactor);
  SPDLOG_INFO("  - dividing flux by {}", strFactor);

  // make vector of reaction IDs from membrane
  std::vector<std::string> reactionID =
      utils::toStdString(doc->reactions.at(membrane->membraneID.c_str()));
  // vector of reaction scale factors to convert flux to concentration
  std::vector<std::string> reactionScaleFactors(reactionID.size(), strFactor);

  reacEval = ReacEval(doc, speciesID, reactionID, reactionScaleFactors);
}

void SimMembrane::evaluate_reactions() {
  assert(reacEval.species_values.size() == fieldA.size() + fieldB.size());
  for (const auto &p : membrane->indexPair) {
    std::size_t ixA = p.first;
    std::size_t ixB = p.second;
    // populate species concentrations: first A, then B
    std::size_t reacIndex = 0;
    for (const auto *fA : fieldA) {
      reacEval.species_values[reacIndex] = fA->conc[ixA];
      ++reacIndex;
    }
    for (const auto *fB : fieldB) {
      reacEval.species_values[reacIndex] = fB->conc[ixB];
      ++reacIndex;
    }
    // evaluate reaction terms
    reacEval.evaluate();
    const std::vector<double> &result = reacEval.getResult();
    // add results to dc/dt: first A, then B
    reacIndex = 0;
    for (auto *fA : fieldA) {
      fA->dcdt[ixA] += result[reacIndex];
      ++reacIndex;
    }
    for (auto *fB : fieldB) {
      fB->dcdt[ixB] += result[reacIndex];
      ++reacIndex;
    }
  }
}

void Simulate::addCompartment(const geometry::Compartment *compartment) {
  SPDLOG_DEBUG("adding compartment {}", compartment->compartmentID);
  simComp.emplace_back(doc, compartment);
  for (auto *f : simComp.back().field) {
    field.push_back(f);
    speciesID.push_back(f->speciesID);
    SPDLOG_DEBUG("  - adding species {}", f->speciesID);
  }
}

void Simulate::addMembrane(geometry::Membrane *membrane) {
  SPDLOG_DEBUG("adding membrane {}", membrane->membraneID);
  simMembrane.emplace_back(doc, membrane);
}

void Simulate::integrateForwardsEuler(double dt) {
  // apply Diffusion operator in all compartments: dc/dt = D ...
  for (auto *f : field) {
    if (f->isSpatial) {
      f->applyDiffusionOperator();
    } else {
      std::fill(f->dcdt.begin(), f->dcdt.end(), 0.0);
    }
  }
  // evaluate reaction terms in all compartments: dc/dt += ...
  for (auto &sim : simComp) {
    sim.evaluate_reactions();
  }
  // evaluate reaction terms in all membranes: dc/dt += ...
  for (auto &sim : simMembrane) {
    sim.evaluate_reactions();
  }
  // for non-spatial species: spatially average dc/dt:
  // roughly equivalent to infinite rate of diffusion
  for (auto *f : field) {
    if (!f->isSpatial) {
      double av_dcdt = std::accumulate(f->dcdt.cbegin(), f->dcdt.cend(),
                                       static_cast<double>(0)) /
                       static_cast<double>(f->dcdt.size());
      std::fill(f->dcdt.begin(), f->dcdt.end(), av_dcdt);
    }
  }
  // forwards Euler timestep: c += dt * (dc/dt) in all compartments
  for (auto *f : field) {
    for (std::size_t i = 0; i < f->conc.size(); ++i) {
      f->conc[i] += dt * f->dcdt[i];
    }
  }
}

QImage Simulate::getConcentrationImage() const {
  if (field.empty()) {
    return QImage();
  }
  QImage img(field[0]->geometry->getCompartmentImage().size(),
             QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  // iterate over compartments
  for (const auto &comp : simComp) {
    if (!comp.field.empty()) {
      // normalise species concentration:
      // max value of each species = max colour intensity
      // with lower bound, so constant zero is still zero
      std::vector<double> max_conc;
      for (const auto *f : comp.field) {
        double m = *std::max_element(f->conc.cbegin(), f->conc.cend());
        max_conc.push_back(m < 1e-15 ? 1.0 : m);
      }
      // equal contribution from each field
      double alpha = 1.0 / static_cast<double>(comp.field.size());
      for (std::size_t i = 0; i < comp.field[0]->geometry->ix.size(); ++i) {
        const QPoint &p = comp.field[0]->geometry->ix[i];
        int r = 0;
        int g = 0;
        int b = 0;
        for (std::size_t i_f = 0; i_f < comp.field.size(); ++i_f) {
          const auto *f = comp.field[i_f];
          double c = alpha * f->conc[i] / max_conc[i_f];
          const auto &col = f->colour;
          r += static_cast<int>(col.red() * c);
          g += static_cast<int>(col.green() * c);
          b += static_cast<int>(col.blue() * c);
        }
        img.setPixel(p, qRgb(r, g, b));
      }
    }
  }
  return img;
}

}  // namespace simulate
