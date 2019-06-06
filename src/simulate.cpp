#include "simulate.h"

void Simulate::compile_reactions(const std::vector<std::string> &species) {
  // init vector of species
  species_values = std::vector<double>(species.size(), 0.0);
  M.clear();
  reac_eval.reserve(doc.model->getNumReactions());

  // get global constants
  std::vector<std::string> constant_names;
  std::vector<double> constant_values;
  // add all *constant* species as constants
  for (unsigned k = 0; k < doc.model->getNumSpecies(); ++k) {
    const auto *spec = doc.model->getSpecies(k);
    if ((spec->isSetConstant() && spec->getConstant()) ||
        (spec->isSetBoundaryCondition() && spec->getBoundaryCondition())) {
      double init_conc = 0;
      // if SBML file specifies amount: convert to concentration
      if (spec->isSetInitialAmount()) {
        double vol =
            doc.model->getCompartment(spec->getCompartment())->getSize();
        init_conc = spec->getInitialAmount() / vol;
      } else {
        init_conc = spec->getInitialConcentration();
      }
      constant_names.push_back(spec->getId());
      constant_values.push_back(init_conc);
    }
  }
  for (unsigned k = 0; k < doc.model->getNumParameters(); ++k) {
    constant_names.push_back(doc.model->getParameter(k)->getId());
    constant_values.push_back(doc.model->getParameter(k)->getValue());
  }
  // also get compartment volumes (the compartmentID may be used in the reaction
  // equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < doc.model->getNumCompartments(); ++k) {
    const auto *comp = doc.model->getCompartment(k);
    constant_names.push_back(comp->getId());
    constant_values.push_back(comp->getSize());
  }

  // process each reaction
  for (int k = 0; k < doc.model->getNumReactions(); ++k) {
    const auto *reac = doc.model->getReaction(k);

    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    std::vector<double> Mrow(species.size(), 0);
    bool isNullReaction = true;
    for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getProduct(k);
      // if it is in the species vector, insert into matrix M
      auto it =
          std::find(species.cbegin(), species.cend(), spec_ref->getSpecies());
      if (it != species.cend()) {
        std::size_t species_index =
            static_cast<std::size_t>(it - species.cbegin());
        isNullReaction = false;
        Mrow[species_index] += spec_ref->getStoichiometry();
        qDebug("M[%lu] += %f", species_index, spec_ref->getStoichiometry());
      }
    }
    for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getReactant(k);
      // if it is in the species vector, insert into matrix M
      auto it =
          std::find(species.cbegin(), species.cend(), spec_ref->getSpecies());
      if (it != species.cend()) {
        std::size_t species_index =
            static_cast<std::size_t>(it - species.cbegin());
        isNullReaction = false;
        Mrow[species_index] -= spec_ref->getStoichiometry();
        qDebug("M[%lu] -= %f", species_index, spec_ref->getStoichiometry());
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

      // inline function calls in expr
      expr = doc.inlineFunctions(expr);

      // get local parameters, append to global constants
      std::vector<std::string> reac_constant_names(constant_names);
      std::vector<double> reac_constant_values(constant_values);
      // append local parameters and their values
      for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
        reac_constant_names.push_back(kin->getLocalParameter(k)->getId());
        reac_constant_values.push_back(kin->getLocalParameter(k)->getValue());
      }
      for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
        reac_constant_names.push_back(kin->getParameter(k)->getId());
        reac_constant_values.push_back(kin->getParameter(k)->getValue());
      }

      for (std::size_t k = 0; k < reac_constant_names.size(); ++k) {
        qDebug("const: %s %f", reac_constant_names[k].c_str(),
               reac_constant_values[k]);
      }

      // compile expression and add to reac_eval vector
      reac_eval.emplace_back(numerics::ReactionEvaluate(
          expr, species, species_values, reac_constant_names,
          reac_constant_values));
    }
  }
  qDebug() << M;
}

std::vector<double> Simulate::evaluate_reactions() {
  std::vector<double> result(species_values.size(), 0.0);
  for (std::size_t j = 0; j < reac_eval.size(); ++j) {
    double r_j = reac_eval[j]();
    for (std::size_t i = 0; i < M.size(); ++i) {
      result[i] += M[i][j] * r_j;
    }
  }
  return result;
}

void Simulate::timestep_1d_euler(double dt) {
  std::vector<double> dcdt = evaluate_reactions();
  for (std::size_t i = 0; i < species_values.size(); ++i) {
    species_values[i] += dcdt[i] * dt;
  }
}

void Simulate::evaluate_reactions(Field &field) {
  qDebug("evaluate_reactions: n_pixels=%lu", field.n_pixels);
  qDebug("evaluate_reactions: n_species=%lu", field.n_species);
  qDebug("evaluate_reactions: M=%lu%lu", M.size(), M[0].size());
  qDebug("evaluate_reactions: n_reacs=%lu", reac_eval.size());
  for (std::size_t ix = 0; ix < field.n_pixels; ++ix) {
    // populate species concentrations
    for (std::size_t s = 0; s < field.n_species; ++s) {
      species_values[s] = field.conc[ix * field.n_species + s];
    }
    for (std::size_t j = 0; j < reac_eval.size(); ++j) {
      // evaluate reaction terms
      double r_j = reac_eval[j]();
      for (std::size_t s = 0; s < field.n_species; ++s) {
        // add results to dcdt
        field.dcdt[ix * field.n_species + s] += M[j][s] * r_j;
      }
    }
  }
}

void Simulate::timestep_2d_euler(Field &field, double dt) {
  field.diffusion_op();
  evaluate_reactions(field);
  for (std::size_t i = 0; i < field.conc.size(); ++i) {
    field.conc[i] += dt * field.dcdt[i];
  }
}
