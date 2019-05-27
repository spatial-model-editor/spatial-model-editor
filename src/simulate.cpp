#include "simulate.h"

void simulate::compile_reactions() {
  // init vector of species with zeros
  species_values = std::vector<double>(doc.model->getNumSpecies(), 0.0);
  // init matrix M with zeros, matrix size: n_species x n_reactions
  M.clear();
  for (unsigned int i = 0; i < doc.model->getNumSpecies(); ++i) {
    M.emplace_back(std::vector<double>(doc.model->getNumReactions(), 0));
  }
  reac_eval.reserve(doc.model->getNumReactions());
  // process each reaction
  for (unsigned int j = 0; j < doc.model->getNumReactions(); ++j) {
    const auto *reac = doc.model->getReaction(j);
    // get mathematical formula
    const auto *kin = reac->getKineticLaw();
    std::string expr = kin->getFormula().c_str();
    // get local variable names and their values
    std::vector<std::string> constant_names;
    std::vector<double> constant_values;
    for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
      constant_names.push_back(kin->getLocalParameter(k)->getId().c_str());
      constant_values.push_back(kin->getLocalParameter(k)->getValue());
    }
    // compile expression and add to reac_eval vector
    reac_eval.emplace_back(numerics::reaction_eval(
        expr, doc.speciesID, species_values, constant_names, constant_values));
    // add a +1 to matrix M for each species produced by this reaction
    for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
      // get product species (ID?)
      std::string spec = reac->getProduct(k)->getSpecies().c_str();
      // convert species ID to species index i
      std::size_t i = doc.speciesIndex.at(spec);
      // insert a +1 at (i,j) in matrix M
      M[i][j] = +1.0;
    }
    // add a -1 to matrix M for each species consumed by this reaction
    for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
      // get product species (ID?)
      std::string spec = reac->getReactant(k)->getSpecies().c_str();
      // convert species ID to species index i
      std::size_t i = doc.speciesIndex.at(spec);
      // insert a -1 at (i,j) in matrix M
      M[i][j] = -1.0;
    }
  }
}

std::vector<double> simulate::evaluate_reactions() {
  std::vector<double> result(species_values.size(), 0.0);
  for (std::size_t i = 0; i < M.size(); ++i) {
    for (std::size_t j = 0; j < reac_eval.size(); ++j) {
      result[i] += M[i][j] * reac_eval[j]();
    }
  }
  return result;
}

void simulate::euler_timestep(double dt) {
  std::vector<double> dcdt = evaluate_reactions();
  for (std::size_t i = 0; i < species_values.size(); ++i) {
    species_values[i] += dcdt[i] * dt;
  }
  qDebug() << species_values;
}
