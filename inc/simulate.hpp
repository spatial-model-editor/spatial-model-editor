// Simple simulation routines
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane
//  - Simulate: forwards Euler integration of a 2d reaction-diffusion model

#pragma once

#include <QImage>

#include "geometry.hpp"
#include "numerics.hpp"
#include "sbml.hpp"
#include "symbolic.hpp"

namespace simulate {

enum class BACKEND { EXPRTK, SYMENGINE, SYMENGINE_LLVM };
std::string strBackend(BACKEND b);

class ReacEval {
 private:
  sbml::SbmlDocWrapper *doc;
  BACKEND backend;
  // vector of exprtk reaction expressions
  std::vector<numerics::ExprEval> reac_eval_exprtk;
  // symengine reaction expression
  symbolic::Symbolic reac_eval_symengine;

  // vector of result of evaluating reactions
  std::vector<double> result;

 public:
  // vector of species concentrations that Reaction expressions will use
  std::vector<double> species_values;
  std::size_t nSpecies = 0;
  ReacEval() = default;
  ReacEval(sbml::SbmlDocWrapper *doc_ptr,
           const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID, BACKEND mathBackend);
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

  explicit SimCompartment(sbml::SbmlDocWrapper *docWrapper,
                          const geometry::Compartment *compartment,
                          BACKEND mathBackend);
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

  SimMembrane(sbml::SbmlDocWrapper *doc_ptr, geometry::Membrane *membrane_ptr,
              BACKEND mathBackend);
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
};

class Simulate {
 private:
  std::vector<SimCompartment> simComp;
  std::vector<SimMembrane> simMembrane;
  sbml::SbmlDocWrapper *doc;
  BACKEND backend;

 public:
  std::vector<geometry::Field *> field;
  std::vector<std::string> speciesID;
  void setMathBackend(BACKEND mathBackend);
  BACKEND getMathBackend() const;
  explicit Simulate(sbml::SbmlDocWrapper *doc_ptr,
                    BACKEND mathBackend = BACKEND::EXPRTK)
      : doc(doc_ptr), backend(mathBackend) {}
  void addCompartment(const geometry::Compartment *compartment);
  void addMembrane(geometry::Membrane *membrane);
  void integrateForwardsEuler(double dt);
  QImage getConcentrationImage();
};

}  // namespace simulate
