// Pixel simulator
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane
//  - PixelSim: forwards Euler simulator

#pragma once

#include <QImage>

#include "basesim.hpp"
#include "symbolic.hpp"

namespace sbml {
class SbmlDocWrapper;
}

namespace geometry {
class Compartment;
class Membrane;
}  // namespace geometry

namespace sim {

class ReacEval {
 private:
  // symengine reaction expression
  symbolic::Symbolic sym;
  // vector of result of evaluating reactions
  std::vector<double> result;

 public:
  // vector of species concentrations that Reaction expressions will use
  std::vector<double> species_values;
  std::size_t nSpecies = 0;
  ReacEval() = default;
  ReacEval(const sbml::SbmlDocWrapper &doc,
           const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID,
           const std::vector<std::string> &reactionScaleFactors);
  void evaluate();
  const std::vector<double> &getResult() const { return result; }
};

class SimCompartment {
 private:
  ReacEval reacEval;
  // species concentrations & corresponding dcdt values
  // ordering: ix, species
  std::vector<double> conc;
  std::vector<double> dcdt;
  // dimensionless diffusion constants for each species
  std::vector<double> diffConstants;
  const geometry::Compartment &comp;
  std::string compartmentId;
  std::vector<std::string> speciesIds;
  std::vector<std::size_t> nonSpatialSpeciesIndices;
  void spatiallyAverageDcdt();

 public:
  explicit SimCompartment(const sbml::SbmlDocWrapper &doc,
                          const geometry::Compartment &compartment);
  // dcdt = result of applying diffusion operator to conc
  void evaluateDiffusionOperator();
  // dcdt += result of applying reaction expressions to conc
  void evaluateReactions();
  void doForwardsEulerTimestep(double dt);
  const std::string &getCompartmentId() const;
  const std::vector<std::string> &getSpeciesIds() const;
  const std::vector<double> &getConcentrations() const;
  const std::vector<QPoint> &getPixels() const;
  std::vector<double> &getDcdt();
};

class SimMembrane {
 private:
  ReacEval reacEval;
  const geometry::Membrane &membrane;
  SimCompartment &compA;
  SimCompartment &compB;

 public:
  SimMembrane(const sbml::SbmlDocWrapper &doc,
              const geometry::Membrane &membrane_ptr, SimCompartment &simCompA,
              SimCompartment &simCompB);
  void evaluateReactions();
};

class PixelSim : public BaseSim {
 private:
  std::vector<SimCompartment> simCompartments;
  std::vector<SimMembrane> simMembranes;
  const sbml::SbmlDocWrapper &doc;

 public:
  explicit PixelSim(const sbml::SbmlDocWrapper &sbmlDoc);
  ~PixelSim() override;
  void doTimestep(double t, double dt) override;
  const std::vector<double> &getConcentrations(
      std::size_t compartmentIndex) const override;
};

}  // namespace sim
