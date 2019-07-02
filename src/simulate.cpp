#include "simulate.h"

namespace simulate {

static std::map<std::string, double> getGlobalConstants(
    sbml::SbmlDocWrapper *doc) {
  std::map<std::string, double> constants;
  // add all *constant* species as constants
  for (unsigned k = 0; k < doc->model->getNumSpecies(); ++k) {
    const auto *spec = doc->model->getSpecies(k);
    if (doc->isSpeciesConstant(spec->getId())) {
      double init_conc = 0;
      // if SBML file specifies amount: convert to concentration
      if (spec->isSetInitialAmount()) {
        double vol =
            doc->model->getCompartment(spec->getCompartment())->getSize();
        init_conc = spec->getInitialAmount() / vol;
      } else {
        init_conc = spec->getInitialConcentration();
      }
      qDebug("ReacEval::ReacEval :: adding constant species '%s' as const",
             spec->getId().c_str());
      constants[spec->getId()] = init_conc;
    }
  }
  for (unsigned k = 0; k < doc->model->getNumParameters(); ++k) {
    if (doc->model->getAssignmentRule(doc->model->getParameter(k)->getId()) ==
        nullptr) {
      constants[doc->model->getParameter(k)->getId()] =
          doc->model->getParameter(k)->getValue();
    }
  }
  // also get compartment volumes (the compartmentID may be used in the reaction
  // equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < doc->model->getNumCompartments(); ++k) {
    const auto *comp = doc->model->getCompartment(k);
    constants[comp->getId()] = comp->getSize();
  }
  return constants;
}

static std::string inlineExpr(sbml::SbmlDocWrapper *doc,
                              const std::string &expr) {
  std::string inlined;
  // inline any Function calls in expr
  inlined = doc->inlineFunctions(expr);

  // inline any Assignment Rules in expr
  inlined = doc->inlineAssignments(inlined);
  return inlined;
}

ReacEval::ReacEval(sbml::SbmlDocWrapper *doc_ptr,
                   const std::vector<std::string> &speciesID,
                   const std::vector<std::string> &reactionID, int nRateRules)
    : doc(doc_ptr) {
  // init vector of species
  species_values = std::vector<double>(speciesID.size(), 0.0);
  result = species_values;
  nSpecies = species_values.size();

  M.clear();
  reac_eval.clear();
  reac_eval.reserve(reactionID.size() + nRateRules);

  // check if any species have a RateRule
  // NOTE: not currently valid if raterule involves species in multiple
  // compartments
  for (std::size_t sIndex = 0; sIndex < speciesID.size(); ++sIndex) {
    const auto *rule = doc->model->getRateRule(speciesID[sIndex]);
    if (rule != nullptr) {
      std::map<std::string, double> constants = getGlobalConstants(doc);
      std::string expr = inlineExpr(doc, rule->getFormula());
      std::vector<double> Mrow(speciesID.size(), 0);
      Mrow[sIndex] = 1.0;
      M.push_back(Mrow);
      ++nReactions;
      // compile expression and add to reac_eval vector
      reac_eval.emplace_back(
          numerics::ExprEval(expr, speciesID, species_values, constants));
      qDebug("ReacEval::ReacEval :: adding rate rule for species %s",
             speciesID[sIndex].c_str());
    }
  }

  // process each reaction
  for (const auto &reacID : reactionID) {
    const auto *reac = doc->model->getReaction(reacID);
    bool isNullReaction = true;

    std::map<std::string, double> constants = getGlobalConstants(doc);

    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    std::vector<double> Mrow(speciesID.size(), 0);
    for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getProduct(k);
      // if it is in the species vector, and not constant, insert into matrix M
      auto it = std::find(speciesID.cbegin(), speciesID.cend(),
                          spec_ref->getSpecies());
      if (it != speciesID.cend() &&
          doc->isSpeciesReactive(spec_ref->getSpecies())) {
        std::size_t species_index =
            static_cast<std::size_t>(it - speciesID.cbegin());
        isNullReaction = false;
        Mrow[species_index] += spec_ref->getStoichiometry();
        qDebug("ReacEval::ReacEval ::   - M[%lu] += %f", species_index,
               spec_ref->getStoichiometry());
      }
    }
    for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getReactant(k);
      // if it is in the species vector, insert into matrix M
      auto it = std::find(speciesID.cbegin(), speciesID.cend(),
                          spec_ref->getSpecies());
      if (it != speciesID.cend() &&
          doc->isSpeciesReactive(spec_ref->getSpecies())) {
        std::size_t species_index =
            static_cast<std::size_t>(it - speciesID.cbegin());
        isNullReaction = false;
        Mrow[species_index] -= spec_ref->getStoichiometry();
        qDebug("ReacEval::ReacEval ::   - M[%lu] -= %f", species_index,
               spec_ref->getStoichiometry());
      }
    }

    if (!isNullReaction) {
      // if matrix row is non-zero, i.e. reaction does something, then insert it
      // into the M matrix, and construct the corresponding reaction term
      M.push_back(Mrow);
      ++nReactions;
      // get mathematical formula
      const auto *kin = reac->getKineticLaw();
      std::string expr = inlineExpr(doc, kin->getFormula());

      // TODO: deal with amount vs concentration issues correctly
      // if getHasOnlySubstanceUnits is true for some (all?) species
      // note: would also need to also do this in the inlining step,
      // and in the stoich matrix factors

      // get local parameters, append to global constants
      // NOTE: if a parameter is set by an assignment rule
      // it should *not* be added as a constant below:
      // (it should no longer be present in expr after inlining)
      // append local parameters and their values
      for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
        if (doc->model->getAssignmentRule(kin->getLocalParameter(k)->getId()) ==
            nullptr) {
          constants[kin->getLocalParameter(k)->getId()] =
              kin->getLocalParameter(k)->getValue();
        }
      }
      for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
        if (doc->model->getAssignmentRule(kin->getParameter(k)->getId()) ==
            nullptr) {
          constants[kin->getParameter(k)->getId()] =
              kin->getParameter(k)->getValue();
        }
      }
      // compile expression and add to reac_eval vector
      reac_eval.emplace_back(
          numerics::ExprEval(expr, speciesID, species_values, constants));
    }
  }
  qDebug("ReacEval::ReacEval ::   - final matrix M:");
  qDebug() << M;
}

void ReacEval::evaluate() {
  std::fill(result.begin(), result.end(), 0.0);
  for (std::size_t j = 0; j < nReactions; ++j) {
    // evaluate reaction term
    double r_j = reac_eval[j]();
    // qDebug("ReacEval::ReacEval :: R[%lu] = %f", j, r_j);
    for (std::size_t s = 0; s < nSpecies; ++s) {
      // add contribution for each species to result
      result[s] += M[j][s] * r_j;
    }
  }
}

SimCompartment::SimCompartment(sbml::SbmlDocWrapper *docWrapper,
                               const geometry::Compartment *compartment)
    : doc(docWrapper), comp(compartment) {
  QString compID = compartment->compartmentID.c_str();
  qDebug("SimCompartment::SimCompartment :: compartment: %s",
         compID.toStdString().c_str());
  std::vector<std::string> speciesID;
  int nRateRules = 0;
  for (const auto &s : doc->species.at(compID)) {
    if (!doc->isSpeciesConstant(s.toStdString())) {
      speciesID.push_back(s.toStdString());
      field.push_back(&doc->mapSpeciesIdToField.at(s));
      qDebug("SimCompartment::SimCompartment :: - adding field: %s",
             s.toStdString().c_str());
      if (doc->model->getRateRule(s.toStdString()) != nullptr) {
        ++nRateRules;
      }
    }
  }
  const auto iter = doc->reactions.find(compID);
  if ((nRateRules == 0) &&
      (iter == doc->reactions.cend() || doc->reactions.at(compID).empty())) {
    // If there are no reactions or RateRules in this compartment: we are done
    qDebug(
        "SimCompartment::SimCompartment ::   - no Reactions or RateRules to "
        "compile.");
    return;
  }
  std::vector<std::string> reactionID;
  if (iter != doc->reactions.cend()) {
    for (const auto &reac : iter->second) {
      reactionID.push_back(reac.toStdString());
    }
  }
  reacEval = ReacEval(doc, speciesID, reactionID, nRateRules);
}

void SimCompartment::evaluate_reactions() {
  // qDebug("SimCompartment::evaluate_reactions : compartment: %s",
  //       field->geometry->compartmentID.c_str());
  if (reacEval.nReactions == 0) {
    // qDebug("SimCompartment::evaluate_reactions :   - no reactions to
    // evaluate");
    return;
  }
  // qDebug("SimCompartment::evaluate_reactions :   - n_pixels=%lu",
  // field->n_pixels);
  // qDebug("SimCompartment::evaluate_reactions :   - n_species=%lu",
  //       reacEval.nSpecies);
  // qDebug("SimCompartment::evaluate_reactions :   - n_reacs=%lu",
  //       reacEval.nReactions);
  for (std::size_t i = 0; i < comp->ix.size(); ++i) {
    // populate species concentrations
    for (std::size_t s = 0; s < field.size(); ++s) {
      reacEval.species_values[s] = field[s]->conc[i];
    }
    // evaluate reaction terms
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
  qDebug("SimMembrane::compile_reactions :: membrane: %s",
         membrane->membraneID.c_str());
  qDebug("SimMembrane::compile_reactions :: compA: %s",
         compA.toStdString().c_str());
  qDebug("SimMembrane::compile_reactions :: compB: %s",
         compB.toStdString().c_str());

  if (doc->reactions.find(membrane->membraneID.c_str()) ==
          doc->reactions.cend() ||
      doc->reactions.at(membrane->membraneID.c_str()).empty()) {
    // If there are no reactions in this membrane: we are done
    // qDebug("SimMembrane::compile_reactions ::   - no reactions to compile.");
    return;
  }

  // make vector of species & fields from compartments A and B
  std::vector<std::string> speciesID;
  for (const auto &spec : doc->species.at(compA)) {
    if (!doc->isSpeciesConstant(spec.toStdString())) {
      speciesID.push_back(spec.toStdString());
      fieldA.push_back(&doc->mapSpeciesIdToField.at(spec));
    }
  }
  for (const auto &spec : doc->species.at(compB)) {
    if (!doc->isSpeciesConstant(spec.toStdString())) {
      speciesID.push_back(spec.toStdString());
      fieldB.push_back(&doc->mapSpeciesIdToField.at(spec));
    }
  }

  // make vector of reactions from membrane
  std::vector<std::string> reactionID;
  for (const auto &reac : doc->reactions.at(membrane->membraneID.c_str())) {
    reactionID.push_back(reac.toStdString());
  }

  reacEval = ReacEval(doc, speciesID, reactionID);
}

void SimMembrane::evaluate_reactions() {
  // qDebug("SimMembrane::evaluate_reactions : membrane: %s",
  //         membrane->membraneID.c_str());
  if (reacEval.nReactions == 0) {
    // qDebug("SimMembrane::evaluate_reactions :   - no reactions to
    // evaluate");
    return;
  }
  // qDebug("SimMembrane::evaluate_reactions :   - n_pixel pairs=%lu",
  //    membrane->indexPair.size());
  // qDebug("SimMembrane::evaluate_reactions :   - n_species=%lu",
  //       reacEval.nSpecies);
  // qDebug("SimMembrane::evaluate_reactions :   - n_reacs=%lu",
  //       reacEval.nReactions);
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

void Simulate::addCompartment(geometry::Compartment *compartment) {
  qDebug("Simulate::addCompartment :: adding compartment %s",
         compartment->compartmentID.c_str());
  simComp.emplace_back(doc, compartment);
  for (auto *f : simComp.back().field) {
    field.push_back(f);
    speciesID.push_back(f->speciesID);
    qDebug("Simulate::addCompartment ::   - adding species %s",
           f->speciesID.c_str());
  }
}

void Simulate::addMembrane(geometry::Membrane *membrane) {
  qDebug("Simulate::addMembrane :: adding membrane %s",
         membrane->membraneID.c_str());
  simMembrane.emplace_back(doc, membrane);
}

void Simulate::integrateForwardsEuler(double dt) {
  // apply Diffusion operator in all compartments: dc/dt = D ...
  for (auto *f : field) {
    f->applyDiffusionOperator();
  }
  // evaluate reaction terms in all compartments: dc/dt += ...
  for (auto &sim : simComp) {
    sim.evaluate_reactions();
  }
  // evaluate reaction terms in all membranes: dc/dt += ...
  for (auto &sim : simMembrane) {
    sim.evaluate_reactions();
  }
  // forwards Euler timestep: c += dt * (dc/dt) in all compartments
  for (auto *f : field) {
    for (std::size_t i = 0; i < f->conc.size(); ++i) {
      f->conc[i] += dt * f->dcdt[i];
    }
  }
}

QImage Simulate::getConcentrationImage() {
  QImage img(field[0]->geometry->getCompartmentImage().size(),
             QImage::Format_ARGB32);
  img.fill(qRgba(0, 0, 0, 0));
  // alpha opacity factor
  double alpha = 1.0;
  // normalise species concentration: max value of any species = max colour
  // intensity
  double max_conc = 0;
  for (const auto *f : field) {
    max_conc =
        std::max(max_conc, *std::max_element(f->conc.cbegin(), f->conc.cend()));
  }
  if (max_conc < 1e-15) {
    max_conc = 1.0;
  }
  for (const auto *f : field) {
    for (std::size_t i = 0; i < f->geometry->ix.size(); ++i) {
      double c = f->conc[i] / max_conc;
      QColor oldCol = img.pixelColor(f->geometry->ix[i]);
      int r = oldCol.red();
      int g = oldCol.green();
      int b = oldCol.blue();
      r += static_cast<int>(f->colour.red() * c * alpha);
      g += static_cast<int>(f->colour.green() * c * alpha);
      b += static_cast<int>(f->colour.blue() * c * alpha);
      img.setPixelColor(
          f->geometry->ix[i],
          QColor(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 255));
    }
  }
  return img;
}

}  // namespace simulate
