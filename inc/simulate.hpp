// Simple simulation routines
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane
//  - Simulate: forwards Euler integration of a 2d reaction-diffusion model

#pragma once

#include <QDebug>
#include <QImage>
#include <QPoint>

#include "geometry.hpp"
#include "numerics.hpp"
#include "sbml.hpp"

namespace simulate {

class ReacEval {
 private:
  sbml::SbmlDocWrapper *doc;
  // vector of reaction expressions
  std::vector<numerics::ExprEval> reac_eval;
  // matrix M_ij of stoichiometric coefficients
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;
  // vector of result of evaluating reactions
  std::vector<double> result;
  bool addStoichCoeff(std::vector<double> &Mrow,
                      const libsbml::SpeciesReference *spec_ref, double sign,
                      const std::vector<std::string> &speciesIDs);

 public:
  // vector of species concentrations that Reaction expressions will use
  std::vector<double> species_values;
  std::size_t nSpecies = 0;
  std::size_t nReactions = 0;
  ReacEval() = default;
  ReacEval(sbml::SbmlDocWrapper *doc_ptr,
           const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID,
           std::size_t nRateRules = 0);
  void evaluate();
  const std::vector<double> &getResult() const { return result; }
};

class SimCompartment {
 private:
  sbml::SbmlDocWrapper *doc;
  const geometry::Compartment *comp;
  ReacEval reacEval;

 public:
  std::vector<geometry::Field *> field;

  SimCompartment(sbml::SbmlDocWrapper *docWrapper,
                 const geometry::Compartment *compartment);
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
};

class SimMembrane {
 private:
  sbml::SbmlDocWrapper *doc;
  ReacEval reacEval;

 public:
  geometry::Membrane *membrane;
  std::vector<geometry::Field *> fieldA;
  std::vector<geometry::Field *> fieldB;

  SimMembrane(sbml::SbmlDocWrapper *doc_ptr, geometry::Membrane *membrane_ptr);
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
};

class Simulate {
 private:
  std::vector<SimCompartment> simComp;
  std::vector<SimMembrane> simMembrane;
  sbml::SbmlDocWrapper *doc;

 public:
  std::vector<geometry::Field *> field;
  std::vector<std::string> speciesID;
  explicit Simulate(sbml::SbmlDocWrapper *doc_ptr) : doc(doc_ptr) {}
  void addCompartment(geometry::Compartment *compartment);
  void addMembrane(geometry::Membrane *membrane);
  void integrateForwardsEuler(double dt);
  QImage getConcentrationImage();
};

}  // namespace simulate
