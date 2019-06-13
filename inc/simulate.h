// Simple simulation routine
//  - compiles reaction equations
//  - 2d reaction-diffusion with forwards Euler

#ifndef SIMULATE_H
#define SIMULATE_H

#include <QDebug>
#include <QImage>
#include <QPoint>

#include "model.h"
#include "numerics.h"
#include "sbml.h"

class Simulate {
 private:
  sbmlDocWrapper *doc;
  Field *field;
  // vector of reaction expressions
  std::vector<numerics::ReactionEvaluate> reac_eval;
  // matrix M_ij of stoichiometric coefficients
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;

 public:
  // vector of species that expressions will use
  std::vector<double> species_values;

  explicit Simulate(sbmlDocWrapper *doc_ptr, Field *field_ptr)
      : doc(doc_ptr), field(field_ptr) {}
  // compile reaction expressions
  void compile_reactions();
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
  // integration timestep: forwards euler: conc += dcdt * dt
  void timestep_2d_euler(double dt);
};

#endif  // SIMULATE_H
