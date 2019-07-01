// Simple simulation routines
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane
//  - Simulate: forwards Euler integration of a 2d reaction-diffusion model

#pragma once

#include <QDebug>
#include <QImage>
#include <QPoint>

#include "geometry.h"
#include "numerics.h"
#include "sbml.h"

namespace simulate {

class ReacEval {
 private:
  SbmlDocWrapper *doc;
  // vector of reaction expressions
  std::vector<numerics::ExprEval> reac_eval;
  // matrix M_ij of stoichiometric coefficients
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;
  // vector of result of evaluating reactions
  std::vector<double> result;

 public:
  // vector of species concentrations that Reaction expressions will use
  std::vector<double> species_values;
  std::size_t nSpecies = 0;
  std::size_t nReactions = 0;
  ReacEval() = default;
  ReacEval(SbmlDocWrapper *doc_ptr, const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID, int nRateRules = 0);
  void evaluate();
  const std::vector<double> &getResult() const { return result; }
};

class SimCompartment {
 private:
  SbmlDocWrapper *doc;
  const geometry::Compartment *comp;
  ReacEval reacEval;

 public:
  std::vector<geometry::Field *> field;

  SimCompartment(SbmlDocWrapper *docWrapper,
                 const geometry::Compartment *compartment);
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
};

class SimMembrane {
 private:
  SbmlDocWrapper *doc;
  ReacEval reacEval;

 public:
  geometry::Membrane *membrane;
  std::vector<geometry::Field *> fieldA;
  std::vector<geometry::Field *> fieldB;

  SimMembrane(SbmlDocWrapper *doc_ptr, geometry::Membrane *membrane_ptr);
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
};

class Simulate {
 private:
  std::vector<SimCompartment> simComp;
  std::vector<SimMembrane> simMembrane;
  SbmlDocWrapper *doc;

 public:
  std::vector<geometry::Field *> field;
  std::vector<std::string> speciesID;
  explicit Simulate(SbmlDocWrapper *doc_ptr) : doc(doc_ptr) {}
  void addCompartment(geometry::Compartment *compartment);
  void addMembrane(geometry::Membrane *membrane);
  void integrateForwardsEuler(double dt);
  QImage getConcentrationImage();
};

}  // namespace simulate
