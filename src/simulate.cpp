#include "simulate.h"

void Simulate::compile_reactions() {
  QString compID = field->geometry->compartmentID.c_str();
  // init vector of species
  species_values = std::vector<double>(field->speciesID.size(), 0.0);
  M.clear();
  reac_eval.reserve(static_cast<std::size_t>(doc->reactions.at(compID).size()));

  // get global constants
  std::vector<std::string> constant_names;
  std::vector<double> constant_values;
  // add all *constant* species as constants
  for (unsigned k = 0; k < doc->model->getNumSpecies(); ++k) {
    const auto *spec = doc->model->getSpecies(k);
    if ((spec->isSetConstant() && spec->getConstant()) ||
        (spec->isSetBoundaryCondition() && spec->getBoundaryCondition())) {
      double init_conc = 0;
      // if SBML file specifies amount: convert to concentration
      if (spec->isSetInitialAmount()) {
        double vol =
            doc->model->getCompartment(spec->getCompartment())->getSize();
        init_conc = spec->getInitialAmount() / vol;
      } else {
        init_conc = spec->getInitialConcentration();
      }
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
  for (const auto &reacID : doc->reactions.at(compID)) {
    const auto *reac = doc->model->getReaction(reacID.toStdString());

    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    std::vector<double> Mrow(field->speciesID.size(), 0);
    bool isNullReaction = true;
    for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getProduct(k);
      // if it is in the species vector, insert into matrix M
      auto it = std::find(field->speciesID.cbegin(), field->speciesID.cend(),
                          spec_ref->getSpecies());
      if (it != field->speciesID.cend()) {
        std::size_t species_index =
            static_cast<std::size_t>(it - field->speciesID.cbegin());
        isNullReaction = false;
        Mrow[species_index] += spec_ref->getStoichiometry();
        qDebug("Simulate::compile_reactions :: M[%lu] += %f", species_index,
               spec_ref->getStoichiometry());
      }
    }
    for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getReactant(k);
      // if it is in the species vector, insert into matrix M
      auto it = std::find(field->speciesID.cbegin(), field->speciesID.cend(),
                          spec_ref->getSpecies());
      if (it != field->speciesID.cend()) {
        std::size_t species_index =
            static_cast<std::size_t>(it - field->speciesID.cbegin());
        isNullReaction = false;
        Mrow[species_index] -= spec_ref->getStoichiometry();
        qDebug("Simulate::compile_reactions :: M[%lu] -= %f", species_index,
               spec_ref->getStoichiometry());
      }
    }

    if (!isNullReaction) {
      // if matrix row is non-zero, i.e. reaction does something, then insert it
      // into the M matrix, and construct the corresponding reaction term
      M.push_back(Mrow);
      // get mathematical formula
      const auto *kin = reac->getKineticLaw();
      std::string expr = kin->getFormula();

      // TODO: deal with amount vs concentration issues correctly
      // if getHasOnlySubstanceUnits is true for some (all?) species
      // note: would also need to also do this in the inlining step,
      // and in the stoich matrix factors

      // inline Function calls in expr
      expr = doc->inlineFunctions(expr);

      // inline Assignment Rules in expr
      expr = doc->inlineAssignments(expr);

      // get local parameters, append to global constants
      // NOTE: if a parameter is set by an assignment rule
      // it should *not* be added as a constant below:
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
        qDebug("Simulate::compile_reactions :: constant: %s %f",
               reac_constant_names[k].c_str(), reac_constant_values[k]);
      }

      // compile expression and add to reac_eval vector
      reac_eval.emplace_back(numerics::ReactionEvaluate(
          expr, field->speciesID, species_values, reac_constant_names,
          reac_constant_values));
    }
  }
  qDebug("Simulate::compile_reactions :: matrix M:");
  qDebug() << M;
}

void Simulate::evaluate_reactions() {
  qDebug("Simulate::evaluate_reactions : Compartment=%s",
         field->geometry->compartmentID.c_str());
  qDebug("Simulate::evaluate_reactions : n_pixels=%lu", field->n_pixels);
  qDebug("Simulate::evaluate_reactions : n_species=%lu", field->n_species);
  qDebug("Simulate::evaluate_reactions : M=%lux%lu", M.size(), M[0].size());
  qDebug("Simulate::evaluate_reactions : n_reacs=%lu", reac_eval.size());
  for (std::size_t ix = 0; ix < field->n_pixels; ++ix) {
    // populate species concentrations
    for (std::size_t s = 0; s < field->n_species; ++s) {
      species_values[s] = field->conc[ix * field->n_species + s];
    }
    for (std::size_t j = 0; j < reac_eval.size(); ++j) {
      // evaluate reaction terms
      double r_j = reac_eval[j]();
      qDebug("R[%lu] = %f", j, r_j);
      for (std::size_t s = 0; s < field->n_species; ++s) {
        // add results to dcdt
        field->dcdt[ix * field->n_species + s] += M[j][s] * r_j;
      }
    }
  }
}

void Simulate::timestep_2d_euler(double dt) {
  field->applyDiffusionOperator();
  evaluate_reactions();
  for (std::size_t i = 0; i < field->conc.size(); ++i) {
    field->conc[i] += dt * field->dcdt[i];
  }
}
