// Simple simulation routines
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane
//  - Simulate: forwards Euler integration of a 2d reaction-diffusion model

#pragma once

#include <QImage>

#include "symbolic.hpp"

namespace sbml {
class SbmlDocWrapper;
}

namespace geometry {
class Field;
class Compartment;
class Membrane;
}  // namespace geometry

namespace simulate {

class ReacEval {
 private:
  // symengine reaction expression
  symbolic::Symbolic reac_eval_symengine;
  // vector of result of evaluating reactions
  std::vector<double> result;

 public:
  // vector of species concentrations that Reaction expressions will use
  std::vector<double> species_values;
  std::size_t nSpecies = 0;
  ReacEval() = default;
  ReacEval(const sbml::SbmlDocWrapper *doc_ptr,
           const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID,
           const std::vector<std::string> &reactionScaleFactors);
  void evaluate();
  const std::vector<double> &getResult() const { return result; }
};

class SimCompartment {
 private:
  std::vector<double> conc;
  std::vector<double> dcdt;
  sbml::SbmlDocWrapper *doc;
  const geometry::Compartment *comp;
  ReacEval reacEval;

 public:
  std::vector<geometry::Field *> field;

  explicit SimCompartment(sbml::SbmlDocWrapper *docWrapper,
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
  void addCompartment(const geometry::Compartment *compartment);
  void addMembrane(geometry::Membrane *membrane);
  void integrateForwardsEuler(double dt);
  QImage getConcentrationImage() const;
};

}  // namespace simulate
