#ifndef SIMULATE_H
#define SIMULATE_H

#include <QDebug>

#include "numerics.h"
#include "sbml.h"

class simulate {
private:
  const sbmlDocWrapper &doc;

public:
  // vector of species that expressions will use
  std::vector<double> species_values;
  // vector of reaction expressions
  std::vector<numerics::reaction_eval> reac_eval;
  // matrix M_ij of +1, -1 or 0
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;

  simulate(const sbmlDocWrapper &doc) : doc(doc) {}
  // compile reaction expressions
  void compile_reactions();
  // vector of result of applying reaction expressions
  std::vector<double> evaluate_reactions();
  // forwards euler explicit integration timestep
  void euler_timestep(double dt);
};

#endif // SIMULATE_H
