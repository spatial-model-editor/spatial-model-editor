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
           const std::vector<std::string> &reactionID);
  void evaluate();
  const std::vector<double> &getResult() const { return result; }
};

class SimCompartment {
 private:
  SbmlDocWrapper *doc;
  ReacEval reacEval;

 public:
  geometry::Field *field;

  SimCompartment(SbmlDocWrapper *doc_ptr, geometry::Field *field_ptr);
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions();
};

class SimMembrane {
 private:
  SbmlDocWrapper *doc;
  ReacEval reacEval;

 public:
  geometry::Membrane *membrane;

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
  // a set of default colours for display purposes
  std::vector<QColor> speciesColour{
      {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
      {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
      {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
      {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
      {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};
  std::vector<geometry::Field *> field;
  std::vector<std::string> speciesID;
  explicit Simulate(SbmlDocWrapper *doc_ptr) : doc(doc_ptr) {}
  void addField(geometry::Field *f);
  void addMembrane(geometry::Membrane *membrane);
  void integrateForwardsEuler(double dt);
  QImage getConcentrationImage();
};

}  // namespace simulate
