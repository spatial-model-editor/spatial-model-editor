#include "simulate.h"

ReacEval::ReacEval(SbmlDocWrapper *doc_ptr,
                   const std::vector<std::string> &speciesID,
                   const std::vector<std::string> &reactionID)
    : doc(doc_ptr) {
  // init vector of species
  species_values = std::vector<double>(speciesID.size(), 0.0);
  result = species_values;
  nSpecies = species_values.size();

  M.clear();
  reac_eval.clear();
  reac_eval.reserve(reactionID.size());

  // get global constants
  std::vector<std::string> constant_names;
  std::vector<double> constant_values;
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
      constant_names.push_back(spec->getId());
      constant_values.push_back(init_conc);
    }
  }
  for (unsigned k = 0; k < doc->model->getNumParameters(); ++k) {
    if (doc->model->getAssignmentRule(doc->model->getParameter(k)->getId()) ==
        nullptr) {
      constant_names.push_back(doc->model->getParameter(k)->getId());
      constant_values.push_back(doc->model->getParameter(k)->getValue());
    }
  }
  // also get compartment volumes (the compartmentID may be used in the reaction
  // equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < doc->model->getNumCompartments(); ++k) {
    const auto *comp = doc->model->getCompartment(k);
    constant_names.push_back(comp->getId());
    constant_values.push_back(comp->getSize());
  }

  // process each reaction
  for (const auto &reacID : reactionID) {
    const auto *reac = doc->model->getReaction(reacID);
    bool isNullReaction = true;

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
          !doc->isSpeciesConstant(spec_ref->getSpecies())) {
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
          !doc->isSpeciesConstant(spec_ref->getSpecies())) {
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
      std::string expr = kin->getFormula();

      // TODO: deal with amount vs concentration issues correctly
      // if getHasOnlySubstanceUnits is true for some (all?) species
      // note: would also need to also do this in the inlining step,
      // and in the stoich matrix factors

      // inline any Function calls in expr
      expr = doc->inlineFunctions(expr);

      // inline any Assignment Rules in expr
      expr = doc->inlineAssignments(expr);

      // get local parameters, append to global constants
      // NOTE: if a parameter is set by an assignment rule
      // it should *not* be added as a constant below:
      // (it should no longer be present in expr after inlining)
      std::vector<std::string> reac_constant_names(constant_names);
      std::vector<double> reac_constant_values(constant_values);
      // append local parameters and their values
      for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
        if (doc->model->getAssignmentRule(kin->getLocalParameter(k)->getId()) ==
            nullptr) {
          reac_constant_names.push_back(kin->getLocalParameter(k)->getId());
          reac_constant_values.push_back(kin->getLocalParameter(k)->getValue());
        }
      }
      for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
        if (doc->model->getAssignmentRule(kin->getParameter(k)->getId()) ==
            nullptr) {
          reac_constant_names.push_back(kin->getParameter(k)->getId());
          reac_constant_values.push_back(kin->getParameter(k)->getValue());
        }
      }

      for (std::size_t k = 0; k < reac_constant_names.size(); ++k) {
        // qDebug("ReacEval::ReacEval ::   - constant: %s %f",
        //        reac_constant_names[k].c_str(), reac_constant_values[k]);
      }

      // compile expression and add to reac_eval vector
      reac_eval.emplace_back(numerics::ExprEval(expr, speciesID, species_values,
                                                reac_constant_names,
                                                reac_constant_values));
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

SimCompartment::SimCompartment(SbmlDocWrapper *doc_ptr, Field *field_ptr)
    : doc(doc_ptr), field(field_ptr) {
  std::vector<std::string> reactionID;
  QString compID = field->geometry->compartmentID.c_str();
  qDebug("SimCompartment::compile_reactions :: compartment: %s",
         compID.toStdString().c_str());

  if (doc->reactions.find(compID) == doc->reactions.cend() ||
      doc->reactions.at(compID).empty()) {
    // If there are no reactions in this compartment: we are done
    qDebug("SimCompartment::compile_reactions ::   - no reactions to compile.");
    return;
  }
  for (const auto &reac : doc->reactions.at(compID)) {
    reactionID.push_back(reac.toStdString());
  }

  reacEval = ReacEval(doc, field->speciesID, reactionID);
}

void SimCompartment::evaluate_reactions() {
  qDebug("SimCompartment::evaluate_reactions : compartment: %s",
         field->geometry->compartmentID.c_str());
  if (reacEval.nReactions == 0) {
    qDebug("SimCompartment::evaluate_reactions :   - no reactions to evaluate");
    return;
  }
  qDebug("SimCompartment::evaluate_reactions :   - n_pixels=%lu",
         field->n_pixels);
  qDebug("SimCompartment::evaluate_reactions :   - n_species=%lu",
         reacEval.nSpecies);
  qDebug("SimCompartment::evaluate_reactions :   - n_reacs=%lu",
         reacEval.nReactions);
  for (std::size_t ix = 0; ix < field->n_pixels; ++ix) {
    // populate species concentrations
    for (std::size_t s = 0; s < field->n_species; ++s) {
      reacEval.species_values[s] = field->conc[ix * field->n_species + s];
    }
    // evaluate reaction terms
    reacEval.evaluate();
    const std::vector<double> &result = reacEval.getResult();
    for (std::size_t s = 0; s < field->n_species; ++s) {
      // add results to dcdt
      field->dcdt[ix * field->n_species + s] += result[s];
    }
  }
}

SimMembrane::SimMembrane(SbmlDocWrapper *doc_ptr, Membrane *membrane_ptr)
    : doc(doc_ptr), membrane(membrane_ptr) {
  std::vector<std::string> speciesID;
  std::vector<std::string> reactionID;
  QString compA = membrane->fieldA->geometry->compartmentID.c_str();
  QString compB = membrane->fieldB->geometry->compartmentID.c_str();
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
    qDebug("SimMembrane::compile_reactions ::   - no reactions to compile.");
    return;
  }

  // make vector of species from compartments A and B
  speciesID = membrane->fieldA->speciesID;
  speciesID.insert(speciesID.end(), membrane->fieldB->speciesID.begin(),
                   membrane->fieldB->speciesID.end());

  // make vector of reactions from membrane
  for (const auto &reac : doc->reactions.at(membrane->membraneID.c_str())) {
    reactionID.push_back(reac.toStdString());
  }

  reacEval = ReacEval(doc, speciesID, reactionID);
}

void SimMembrane::evaluate_reactions() {
  qDebug("SimMembrane::evaluate_reactions : membrane: %s",
         membrane->membraneID.c_str());
  if (reacEval.nReactions == 0) {
    qDebug("SimMembrane::evaluate_reactions :   - no reactions to evaluate");
    return;
  }
  qDebug("SimMembrane::evaluate_reactions :   - n_pixel pairs=%lu",
         membrane->indexPair.size());
  qDebug("SimMembrane::evaluate_reactions :   - n_species=%lu",
         reacEval.nSpecies);
  qDebug("SimMembrane::evaluate_reactions :   - n_reacs=%lu",
         reacEval.nReactions);
  for (const auto &p : membrane->indexPair) {
    std::size_t ixA = p.first;
    std::size_t ixB = p.second;
    // populate species concentrations: first A, then B
    std::size_t reacIndex = 0;
    for (std::size_t s = 0; s < membrane->fieldA->n_species; ++s) {
      reacEval.species_values[reacIndex++] =
          membrane->fieldA->conc[ixA * membrane->fieldA->n_species + s];
    }
    for (std::size_t s = 0; s < membrane->fieldB->n_species; ++s) {
      reacEval.species_values[reacIndex++] =
          membrane->fieldB->conc[ixB * membrane->fieldB->n_species + s];
    }
    // evaluate reaction terms
    reacEval.evaluate();
    const std::vector<double> &result = reacEval.getResult();
    // add results to dc/dt: first A, then B
    reacIndex = 0;
    for (std::size_t s = 0; s < membrane->fieldA->n_species; ++s) {
      membrane->fieldA->dcdt[ixA * membrane->fieldA->n_species + s] +=
          result[reacIndex++];
    }
    for (std::size_t s = 0; s < membrane->fieldB->n_species; ++s) {
      membrane->fieldB->dcdt[ixB * membrane->fieldB->n_species + s] +=
          result[reacIndex++];
    }
  }
}

void Simulate::addField(Field *f) {
  field.push_back(f);
  speciesID.insert(speciesID.end(), f->speciesID.begin(), f->speciesID.end());
  simComp.emplace_back(doc, f);
}

void Simulate::addMembrane(Membrane *membrane) {
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
  double alpha = 0.75;
  // offset to go from species index in a field to the species index used in
  // speciesID here, which includes all species in the model
  std::size_t s_offset = 0;
  // normalise species concentration: max value of any species = max colour
  // intensity
  double max_conc = 0;
  for (auto *f : field) {
    max_conc =
        std::max(max_conc, *std::max_element(f->conc.cbegin(), f->conc.cend()));
  }
  for (auto *f : field) {
    for (std::size_t i = 0; i < f->geometry->ix.size(); ++i) {
      int r = 0;
      int g = 0;
      int b = 0;
      for (std::size_t s = 0; s < f->n_species; ++s) {
        double c = f->conc[i * f->n_species + s] / max_conc;
        r += static_cast<int>((speciesColour[s + s_offset].red() * c) * alpha);
        g +=
            static_cast<int>((speciesColour[s + s_offset].green() * c) * alpha);
        b += static_cast<int>((speciesColour[s + s_offset].blue() * c) * alpha);
        img.setPixel(
            f->geometry->ix[i],
            QColor(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 255)
                .rgba());
      }
    }
    s_offset += f->speciesID.size();
  }
  return img;
}
